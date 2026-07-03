#include "RTSSelectionSubsystem.h"
#include "RTSSelectable.h"
#include "RTSCommandSubsystem.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Interfaces/RTSCommandInterface.h"
#include "Data/RTSCommandGridAsset.h"
#include "Data/RTSCommandButton.h"
#include "Commands/RTSUnitCommands.h"
#include "Components/MassBattleAgentComponent.h"
#include "Fragments/SubType.h"
#include "Tasks/MassBattleBPTaskAgentsMoveTo.h"
#include "Tasks/MassBattleBPTaskAgentsChaseAttack.h"
#include "Interfaces/MassBattleAgentInterface.h"
#include "FuncLibs/MassBattleFuncLib.h"

DEFINE_LOG_CATEGORY(LogORTSSelection);

namespace
{
	constexpr int32 MaxSynchronousFormationEntities = 512;

	FString GetActorGroupKey(const AActor* Actor)
	{
		if (!Actor)
		{
			return FString();
		}

		if (const URTSSelectable* Selectable = Actor->FindComponentByClass<URTSSelectable>())
		{
			if (!Selectable->SelectionGroupKey.IsEmpty())
			{
				return Selectable->SelectionGroupKey;
			}
		}

		return Actor->GetClass() ? Actor->GetClass()->GetName() : Actor->GetName();
	}
}

void URTSSelectionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

    // C++ Auto-Config Grid (Transient)
    // If no grid is provided, use the built-in MassBattle unit grid.
    if (DefaultEntityGrid.IsNull())
    {
        UE_LOG(LogORTSSelection, Log, TEXT("Selection: Auto-configuring transient default grid."));
        URTSUnitCommandGrid* TransientGrid = NewObject<URTSUnitCommandGrid>(this, TEXT("TransientDefaultUnitCommandGrid"));

        DefaultEntityGrid = TransientGrid;
        DefaultGridNative = TransientGrid; // Keep it alive and accessible
    }
}

void URTSSelectionSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URTSSelectionSubsystem::SetSelectedUnits(const TArray<AActor*>& InActors, const TArray<FEntityHandle>& InEntities, ERTSSelectionModifier Modifier)
{
    TArray<AActor*> FinalActors = InActors;
    TArray<FEntityHandle> FinalEntities = InEntities;

    // Strategic Resolution: Convert Actors to Entities if they are Proxies
    for (int32 i = FinalActors.Num() - 1; i >= 0; i--)
    {
        AActor* Actor = FinalActors[i];
        if (Actor)
        {
            if (UMassBattleAgentComponent* MassAgent = Actor->FindComponentByClass<UMassBattleAgentComponent>())
            {
                FEntityHandle ProxiedEntity = MassAgent->GetEntityHandle();
                if (ProxiedEntity.Index != 0)
                {
                    FinalEntities.AddUnique(ProxiedEntity);
                    FinalActors.RemoveAt(i);
                }
            }
        }
    }

	// 1. Update Internal State
	if (Modifier == ERTSSelectionModifier::Replace)
	{
		if (SelectedEntities.Num() > 0)
		{
			UMassBattleFuncLib::DeselectAgents(this, SelectedEntities, ESelectState::All);
		}
		
		SelectedActors = FinalActors;
		SelectedEntities = FinalEntities;
		
		if (SelectedEntities.Num() > 0)
		{
			UMassBattleFuncLib::SelectAgents(this, SelectedEntities, ESelectState::Selected);
		}
	}
	else if (Modifier == ERTSSelectionModifier::Add)
	{
		for (AActor* Actor : FinalActors) SelectedActors.AddUnique(Actor);
		for (const FEntityHandle& Handle : FinalEntities) SelectedEntities.AddUnique(Handle);
		if (FinalEntities.Num() > 0)
		{
			UMassBattleFuncLib::SelectAgents(this, FinalEntities, ESelectState::Selected);
		}
	}
	else if (Modifier == ERTSSelectionModifier::Remove)
	{
		for (AActor* Actor : FinalActors) SelectedActors.Remove(Actor);
		for (const FEntityHandle& Handle : FinalEntities) SelectedEntities.Remove(Handle);
		if (FinalEntities.Num() > 0)
		{
			UMassBattleFuncLib::DeselectAgents(this, FinalEntities, ESelectState::All);
		}
	}

	// 2. Generate View Data
	FRTSSelectionView View;
	int32 TotalCount = SelectedActors.Num() + SelectedEntities.Num();

	if (TotalCount == 0)
	{
		View.Mode = ERTSSelectionMode::Single;
	}
	else if (TotalCount == 1)
	{
		View.Mode = ERTSSelectionMode::Single;
		if (SelectedActors.Num() > 0) View.SingleUnit = CreateUnitDataFromActor(SelectedActors[0]);
		else View.SingleUnit = CreateUnitDataFromEntity(SelectedEntities[0]);
        View.Items.Add(View.SingleUnit);
	}
	else if (TotalCount <= ListModeMaxCount)
	{
		View.Mode = ERTSSelectionMode::List;
		for (AActor* Actor : SelectedActors) View.Items.Add(CreateUnitDataFromActor(Actor));
		for (const FEntityHandle& Handle : SelectedEntities) View.Items.Add(CreateUnitDataFromEntity(Handle));
		// 按类型名排序，保证同类型单位连续显示
		View.Items.Sort([](const FRTSUnitData& A, const FRTSUnitData& B){ return A.Name < B.Name; });
	}
	else
	{
		View.Mode = ERTSSelectionMode::Summary;
		TMap<FString, FRTSUnitData> GroupMap;

		for (AActor* Actor : SelectedActors)
		{
			FRTSUnitData Data = CreateUnitDataFromActor(Actor);
			FRTSUnitData& Group = GroupMap.FindOrAdd(Data.Name);
			if (Group.Count == 0 || Group.Name.IsEmpty()) { Group = Data; Group.Count = 0; }
			Group.Count++;
		}

		for (const FEntityHandle& Handle : SelectedEntities)
		{
			FRTSUnitData Data = CreateUnitDataFromEntity(Handle);
			FRTSUnitData& Group = GroupMap.FindOrAdd(Data.Name);
			if (Group.Count == 0 || Group.Name.IsEmpty()) { Group = Data; Group.Count = 0; }
			Group.Count++;
		}

		for (auto& Pair : GroupMap) View.Items.Add(Pair.Value);
		// Summary 模式：按类型名字母排序，保证 City1→City2→... MassUnit_SubType0→... 依次连续
		View.Items.Sort([](const FRTSUnitData& A, const FRTSUnitData& B){ return A.Name < B.Name; });
	}

	// --- Tab Cycling ---
	AvailableGroupKeys.Reset();
	for(const auto& Item : View.Items) AvailableGroupKeys.AddUnique(Item.Name);
	AvailableGroupKeys.Sort();

	if (CurrentGroupIndex >= AvailableGroupKeys.Num()) CurrentGroupIndex = 0;
	if (AvailableGroupKeys.IsValidIndex(CurrentGroupIndex)) View.ActiveGroupKey = AvailableGroupKeys[CurrentGroupIndex];

	OnSelectionChanged.Broadcast(View);

    // --- Grid Synchronization ---
    // 核心设计：ActiveGroupKey 就是 TypeName（如 "City1", "MassUnit_SubType0"）
    // 直接用它查 Grid 表，不绕路遍历实体句柄
    URTSCommandGridAsset* NewGrid = nullptr;
    const FString& ActiveKey = View.ActiveGroupKey;

    if (!ActiveKey.IsEmpty())
    {
        // 路径A: Actor 组 —— 在选中 Actor 里找 ActiveKey 对应的 Actor，取其 Grid
        for (AActor* Actor : SelectedActors)
        {
            if (Actor && GetActorGroupKey(Actor) == ActiveKey
                && Actor->Implements<URTSCommandInterface>())
            {
                NewGrid = IRTSCommandInterface::Execute_GetCommandGrid(Actor);
                break;
            }
        }

        // Entity groups use the built-in MassBattle default grid below.
    }

    // 路径C: 兜底默认 Grid（士兵移动/攻击/停止）
    if (!NewGrid && !DefaultEntityGrid.IsNull() && (SelectedActors.Num() > 0 || SelectedEntities.Num() > 0))
    {
        NewGrid = DefaultEntityGrid.LoadSynchronous();
    }

    OnCommandNavigationRequested.Broadcast(NewGrid);
    UE_LOG(LogORTSSelection, Log, TEXT("Selection: Modifier=%d Actors=%d Entities=%d ActiveKey=%s Grid=>%s"),
        (int32)Modifier, SelectedActors.Num(), SelectedEntities.Num(),
        *ActiveKey, NewGrid ? *NewGrid->GetName() : TEXT("NULL"));
}


void URTSSelectionSubsystem::ClearSelection()
{
	SetSelectedUnits(TArray<AActor*>(), TArray<FEntityHandle>(), ERTSSelectionModifier::Replace);
}

void URTSSelectionSubsystem::CycleGroup()
{
	if (AvailableGroupKeys.Num() <= 1) return;

	CurrentGroupIndex++;
	if (CurrentGroupIndex >= AvailableGroupKeys.Num()) CurrentGroupIndex = 0;
	SetSelectedUnits(SelectedActors, SelectedEntities, ERTSSelectionModifier::Replace);
}

void URTSSelectionSubsystem::RemoveUnit(const FRTSUnitData& UnitData)
{
	TArray<AActor*> ActorsToRemove;
	TArray<FEntityHandle> EntitiesToRemove;

	if (UnitData.ActorPtr) ActorsToRemove.Add(UnitData.ActorPtr);
	else if (UnitData.EntityHandle.Index != 0) EntitiesToRemove.Add(UnitData.EntityHandle);
	else
	{
		for (AActor* Act : SelectedActors) if (Act && GetActorGroupKey(Act) == UnitData.Name) ActorsToRemove.Add(Act);
	}

	SetSelectedUnits(ActorsToRemove, EntitiesToRemove, ERTSSelectionModifier::Remove);
}

void URTSSelectionSubsystem::SelectGroup(const FString& GroupKey)
{
	TArray<AActor*> NewActors;
	TArray<FEntityHandle> NewEntities;

	for (AActor* Act : SelectedActors) if (Act && GetActorGroupKey(Act) == GroupKey) NewActors.Add(Act);
	for (const FEntityHandle& Handle : SelectedEntities) 
    {
        FRTSUnitData Data = CreateUnitDataFromEntity(Handle);
        if (Data.Name == GroupKey) NewEntities.Add(Handle);
    }

	SetSelectedUnits(NewActors, NewEntities, ERTSSelectionModifier::Replace);
}

FRTSUnitData URTSSelectionSubsystem::CreateUnitDataFromActor(AActor* Actor)
{
	FRTSUnitData Data;
	if (Actor)
	{
		Data.Name = GetActorGroupKey(Actor);
		Data.ActorPtr = Actor;
		Data.bIsMassEntity = false;
		
		if (auto Selectable = Actor->FindComponentByClass<URTSSelectable>())
		{
			Data.Icon = Selectable->Icon;
			Data.Health = Selectable->Health;
			Data.MaxHealth = Selectable->MaxHealth;
			Data.Energy = Selectable->Energy;
			Data.MaxEnergy = Selectable->MaxEnergy;
			Data.Shield = Selectable->Shield;
			Data.MaxShield = Selectable->MaxShield;
		}
	}
	return Data;
}

FRTSUnitData URTSSelectionSubsystem::CreateUnitDataFromEntity(const FEntityHandle& Handle)
{
	FRTSUnitData Data;
	Data.bIsMassEntity = true;
	Data.EntityHandle = Handle;

    UWorld* World = GetWorld();
    if (!World) return Data;

    // 普通 Mass 单位 —— 读取 FSubType.Index 作为分组 Key
    if (UMassEntitySubsystem* MassSys = World->GetSubsystem<UMassEntitySubsystem>())
    {
        FMassEntityManager& EM = MassSys->GetMutableEntityManager();
        if (Handle.Index > 0)
        {
            FMassEntityHandle NativeHandle(Handle.Index, Handle.Serial);
            if (EM.IsEntityActive(NativeHandle))
            {
                if (const FSubType* SubFrag = EM.GetFragmentDataPtr<FSubType>(NativeHandle))
                {
                    Data.Name = FString::Printf(TEXT("MassUnit_SubType%d"), SubFrag->Index);
                    return Data;
                }
            }
        }
    }

    Data.Name = TEXT("Mass Unit");
	return Data;
}


void URTSSelectionSubsystem::IssueCommand(FGameplayTag CommandTag)
{
    UE_LOG(LogTemp, Log, TEXT("RTSSelectionSubsystem: Command %s Issued to Current Selection."), *CommandTag.ToString());

    bool bHandledByCommandSystem = false;
    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (URTSCommandSubsystem* SignalHub = LP->GetSubsystem<URTSCommandSubsystem>())
        {
            SignalHub->IssueCommand(CommandTag, nullptr);
            bHandledByCommandSystem = true;
        }
    }

    if (!bHandledByCommandSystem)
    {
        for (AActor* Actor : SelectedActors)
        {
            if (Actor && Actor->Implements<URTSCommandInterface>())
            {
                IRTSCommandInterface::Execute_ExecuteCommand(Actor, CommandTag);
            }
        }
    }

    RequestCommandRefresh();
}

void URTSSelectionSubsystem::IssueCommandWithLocation(FGameplayTag CommandTag, FVector Location)
{
    UE_LOG(LogTemp, Log, TEXT("RTSSelectionSubsystem: Command %s Issued with Location %s"), *CommandTag.ToString(), *Location.ToString());

    bool bHandledByCommandSystem = false;
    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (URTSCommandSubsystem* SignalHub = LP->GetSubsystem<URTSCommandSubsystem>())
        {
            SignalHub->IssueCommandWithLocation(CommandTag, Location);
            bHandledByCommandSystem = true;
        }
    }

    if (!bHandledByCommandSystem)
    {
        // 回退：只对 Actor 实现兼容
        for (AActor* Actor : SelectedActors)
        {
            if (Actor && Actor->Implements<URTSCommandInterface>())
            {
                IRTSCommandInterface::Execute_ExecuteCommandWithLocation(Actor, CommandTag, Location);
            }
        }
    }

    RequestCommandRefresh();
}

void URTSSelectionSubsystem::IssueCommandWithTarget(FGameplayTag CommandTag, AActor* TargetActor)
{
    UE_LOG(LogTemp, Log, TEXT("RTSSelectionSubsystem: Command %s Issued with TargetActor %s"), *CommandTag.ToString(), TargetActor ? *TargetActor->GetName() : TEXT("NULL"));

    bool bHandledByCommandSystem = false;
    if (ULocalPlayer* LP = GetLocalPlayer())
    {
        if (URTSCommandSubsystem* SignalHub = LP->GetSubsystem<URTSCommandSubsystem>())
        {
            SignalHub->IssueCommandWithTarget(CommandTag, TargetActor);
            bHandledByCommandSystem = true;
        }
    }

    if (!bHandledByCommandSystem)
    {
        // 回退：只对 Actor 实现兼容
        for (AActor* Actor : SelectedActors)
        {
            if (Actor && Actor->Implements<URTSCommandInterface>())
            {
                IRTSCommandInterface::Execute_ExecuteCommandWithTarget(Actor, CommandTag, TargetActor);
            }
        }
    }

    RequestCommandRefresh();
}

AActor* URTSSelectionSubsystem::GetActiveActor() const
{
    if (SelectedActors.Num() == 0) return nullptr;
    if (AvailableGroupKeys.IsValidIndex(CurrentGroupIndex))
    {
        const FString& ActiveKey = AvailableGroupKeys[CurrentGroupIndex];
        for (AActor* Actor : SelectedActors)
        {
            if (Actor && GetActorGroupKey(Actor) == ActiveKey) return Actor;
        }
    }
    return SelectedActors[0];
}
