#include "RTSSelectionSubsystem.h"
#include "RTSInputPanelSettings.h"
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
#include "Fragments/Health.h"
#include "Fragments/SubType.h"
#include "Tasks/MassBattleBPTaskAgentsMoveTo.h"
#include "Tasks/MassBattleBPTaskAgentsChaseAttack.h"
#include "Interfaces/MassBattleAgentInterface.h"
#include "FuncLibs/MassBattleFuncLib.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"

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

	FString GetSelectionUnitGroupKey(const FRTSUnitData& Data)
	{
		return Data.GroupKey.IsEmpty() ? Data.Name : Data.GroupKey;
	}

	FString MakeMassSubtypeGroupKey(int32 SubTypeIndex)
	{
		return FString::Printf(TEXT("MassUnit.SubType.%02d"), SubTypeIndex);
	}

	bool TryParseMassSubtypeGroupKey(const FString& GroupKey, int32& OutSubTypeIndex)
	{
		const FString Prefix = TEXT("MassUnit.SubType.");
		if (!GroupKey.StartsWith(Prefix))
		{
			return false;
		}

		return LexTryParseString(OutSubTypeIndex, *GroupKey.RightChop(Prefix.Len()));
	}

	int32 GetProtocolSubTypeIndex(const FRTSMassUnitTypeProtocol& Protocol, int32 ArrayIndex)
	{
		return Protocol.SubTypeIndex != INDEX_NONE ? Protocol.SubTypeIndex : ArrayIndex;
	}

	const FRTSMassUnitTypeProtocol* FindMassUnitTypeProtocolByIndex(const URTSInputPanelSettings* Settings, int32 SubTypeIndex)
	{
		if (!Settings)
		{
			return nullptr;
		}

		for (int32 i = 0; i < Settings->MassUnitTypeProtocols.Num(); ++i)
		{
			const FRTSMassUnitTypeProtocol& Protocol = Settings->MassUnitTypeProtocols[i];
			if (GetProtocolSubTypeIndex(Protocol, i) == SubTypeIndex)
			{
				return &Protocol;
			}
		}

		return nullptr;
	}

	const FRTSMassUnitTypeProtocol* FindMassUnitTypeProtocolByKey(const URTSInputPanelSettings* Settings, const FString& TypeKey, int32& OutSubTypeIndex)
	{
		if (!Settings || TypeKey.IsEmpty())
		{
			return nullptr;
		}

		for (int32 i = 0; i < Settings->MassUnitTypeProtocols.Num(); ++i)
		{
			const FRTSMassUnitTypeProtocol& Protocol = Settings->MassUnitTypeProtocols[i];
			const int32 ProtocolIndex = GetProtocolSubTypeIndex(Protocol, i);
			const FString ConfiguredKey = Protocol.TypeKey.TrimStartAndEnd();
			const FString FallbackKey = MakeMassSubtypeGroupKey(ProtocolIndex);
			if (ConfiguredKey == TypeKey || FallbackKey == TypeKey)
			{
				OutSubTypeIndex = ProtocolIndex;
				return &Protocol;
			}
		}

		return nullptr;
	}

	const FRTSMassUnitTypeProtocol* ResolveMassUnitTypeProtocol(const URTSInputPanelSettings* Settings, const FString& TypeKey, int32& OutSubTypeIndex)
	{
		if (TryParseMassSubtypeGroupKey(TypeKey, OutSubTypeIndex))
		{
			return FindMassUnitTypeProtocolByIndex(Settings, OutSubTypeIndex);
		}

		return FindMassUnitTypeProtocolByKey(Settings, TypeKey, OutSubTypeIndex);
	}

	FString GetMassProtocolTypeKey(const FRTSMassUnitTypeProtocol* Protocol, int32 SubTypeIndex)
	{
		if (Protocol)
		{
			const FString ConfiguredKey = Protocol->TypeKey.TrimStartAndEnd();
			if (!ConfiguredKey.IsEmpty())
			{
				return ConfiguredKey;
			}
		}

		return MakeMassSubtypeGroupKey(SubTypeIndex);
	}

	FString GetDefaultMassSubtypeDisplayName(int32 SubTypeIndex)
	{
		static const TCHAR* DefaultNames[] =
		{
			TEXT("Rifle Section"),
			TEXT("Assault Section"),
			TEXT("Machine Gun Team"),
			TEXT("Mortar Team"),
			TEXT("Anti-Tank Team"),
			TEXT("Engineer Squad"),
			TEXT("Radio Operator"),
			TEXT("Field Medic"),
			TEXT("Scout Team"),
			TEXT("Sniper Team"),
			TEXT("Light Tank"),
			TEXT("Medium Tank"),
			TEXT("Heavy Tank"),
			TEXT("Armored Car"),
			TEXT("Halftrack"),
			TEXT("Artillery Crew"),
			TEXT("Anti-Air Crew"),
			TEXT("Command Squad"),
			TEXT("Naval Infantry"),
			TEXT("Mountain Troops"),
			TEXT("Cavalry Patrol"),
			TEXT("Flame Team"),
			TEXT("Recon Platoon"),
			TEXT("Supply Detail"),
			TEXT("Transport Convoy"),
			TEXT("Tank Destroyer"),
			TEXT("Rocket Battery"),
			TEXT("Airborne Squad"),
			TEXT("Security Detail"),
			TEXT("Veteran Squad"),
			TEXT("Reserve Squad"),
			TEXT("Headquarters")
		};

		if (SubTypeIndex >= 0 && SubTypeIndex < UE_ARRAY_COUNT(DefaultNames))
		{
			return DefaultNames[SubTypeIndex];
		}

		return FString::Printf(TEXT("SubType %02d"), SubTypeIndex);
	}

	FString GetDefaultMassSubtypeRole(int32 SubTypeIndex)
	{
		static const TCHAR* DefaultRoles[] =
		{
			TEXT("Line Infantry"),
			TEXT("Shock Infantry"),
			TEXT("Suppression Team"),
			TEXT("Indirect Fire"),
			TEXT("Anti Armor"),
			TEXT("Combat Engineer"),
			TEXT("Command Support"),
			TEXT("Medical Support"),
			TEXT("Recon"),
			TEXT("Precision Infantry"),
			TEXT("Light Armor"),
			TEXT("Battle Tank"),
			TEXT("Heavy Armor"),
			TEXT("Recon Vehicle"),
			TEXT("Transport"),
			TEXT("Indirect Fire"),
			TEXT("Air Defense"),
			TEXT("Command"),
			TEXT("Amphibious Infantry"),
			TEXT("Rough Terrain Infantry"),
			TEXT("Fast Recon"),
			TEXT("Close Assault"),
			TEXT("Recon"),
			TEXT("Logistics"),
			TEXT("Logistics"),
			TEXT("Anti Armor"),
			TEXT("Rocket Artillery"),
			TEXT("Elite Infantry"),
			TEXT("Garrison"),
			TEXT("Elite Infantry"),
			TEXT("Militia"),
			TEXT("Command Node")
		};

		if (SubTypeIndex >= 0 && SubTypeIndex < UE_ARRAY_COUNT(DefaultRoles))
		{
			return DefaultRoles[SubTypeIndex];
		}

		return TEXT("Combat Unit");
	}

	FName MakeDefaultAnnouncerId(int32 SubTypeIndex, const FString& DisplayName)
	{
		FString Sanitized = DisplayName;
		Sanitized.ReplaceInline(TEXT(" "), TEXT(""));
		Sanitized.ReplaceInline(TEXT("-"), TEXT(""));
		Sanitized.ReplaceInline(TEXT("."), TEXT(""));
		Sanitized.ReplaceInline(TEXT("_"), TEXT(""));

		if (Sanitized.IsEmpty())
		{
			Sanitized = FString::Printf(TEXT("SubType%02d"), SubTypeIndex);
		}

		return FName(*FString::Printf(TEXT("Unit.%s"), *Sanitized));
	}

	UTexture2D* LoadDefaultUnitPanelIconBySeed(uint32 Seed)
	{
		static const TCHAR* DefaultIconFiles[] =
		{
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_01.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_02.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_03.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_04.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_05.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_06.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_07.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_08.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_09.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_10.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_11.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_12.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_13.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_14.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_15.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_16.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_17.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_18.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_19.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_20.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_21.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_22.png"),
			TEXT("UnitIcons/Germany/Germany_Unit_Icon_23.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_01.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_02.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_03.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_04.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_05.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_06.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_07.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_08.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_09.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_10.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_11.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_12.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_13.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_14.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_15.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_16.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_17.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_18.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_19.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_20.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_21.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_22.png"),
			TEXT("UnitIcons/Japan/Japan_Unit_Icon_23.png"),
		};

		static TMap<int32, UTexture2D*> IconCache;

		const int32 IconIndex = static_cast<int32>(Seed % UE_ARRAY_COUNT(DefaultIconFiles));
		if (UTexture2D** CachedTexture = IconCache.Find(IconIndex))
		{
			return *CachedTexture;
		}

		const FString IconPath = FPaths::Combine(
			FPaths::ProjectPluginsDir(),
			TEXT("RTSInputSystem"),
			TEXT("Content"),
			TEXT("Portraits"),
			TEXT("Source"),
			DefaultIconFiles[IconIndex]
		);

		UTexture2D* Texture = nullptr;
		if (IFileManager::Get().FileExists(*IconPath))
		{
			Texture = FImageUtils::ImportFileAsTexture2D(IconPath);
			if (Texture)
			{
				Texture->AddToRoot();
				Texture->SRGB = true;
			}
		}

		IconCache.Add(IconIndex, Texture);
		return Texture;
	}

	UTexture2D* LoadDefaultUnitAvatar()
	{
		static UTexture2D* CachedAvatar = nullptr;
		static bool bAttemptedLoad = false;

		if (bAttemptedLoad)
		{
			return CachedAvatar;
		}

		bAttemptedLoad = true;
		const FString AvatarPath = FPaths::Combine(
			FPaths::ProjectPluginsDir(),
			TEXT("RTSInputSystem"),
			TEXT("Content"),
			TEXT("Portraits"),
			TEXT("Source"),
			TEXT("RTS_UnitAvatar_Placeholder_256x512.png")
		);

		if (IFileManager::Get().FileExists(*AvatarPath))
		{
			CachedAvatar = FImageUtils::ImportFileAsTexture2D(AvatarPath);
			if (CachedAvatar)
			{
				CachedAvatar->AddToRoot();
				CachedAvatar->SRGB = true;
			}
		}

		return CachedAvatar;
	}

	UTexture2D* LoadDefaultUnitAvatarBySeed(uint32 Seed)
	{
		if (UTexture2D* UnitIcon = LoadDefaultUnitPanelIconBySeed(Seed))
		{
			return UnitIcon;
		}

		return LoadDefaultUnitAvatar();
	}

	UTexture2D* LoadConfiguredTexture(const TSoftObjectPtr<UTexture2D>& Texture)
	{
		return Texture.IsNull() ? nullptr : Texture.LoadSynchronous();
	}

	void EnsureSelectionDataDefaults(FRTSUnitData& Data, int32 SubTypeIndex, uint32 IconSeed)
	{
		if (Data.Name.TrimStartAndEnd().IsEmpty())
		{
			Data.Name = SubTypeIndex != INDEX_NONE
				? GetDefaultMassSubtypeDisplayName(SubTypeIndex)
				: TEXT("Mass Unit");
		}

		if (Data.GroupKey.TrimStartAndEnd().IsEmpty())
		{
			Data.GroupKey = SubTypeIndex != INDEX_NONE
				? MakeMassSubtypeGroupKey(SubTypeIndex)
				: FString::Printf(TEXT("MassUnit.Entity.%u"), IconSeed);
		}

		if (Data.TypeKey.TrimStartAndEnd().IsEmpty())
		{
			Data.TypeKey = Data.GroupKey;
		}

		if (Data.Role.TrimStartAndEnd().IsEmpty())
		{
			Data.Role = SubTypeIndex != INDEX_NONE
				? GetDefaultMassSubtypeRole(SubTypeIndex)
				: TEXT("Combat Unit");
		}

		if (Data.AnnouncerId.IsNone())
		{
			Data.AnnouncerId = MakeDefaultAnnouncerId(SubTypeIndex, Data.Name);
		}

		if (!Data.Icon)
		{
			Data.Icon = LoadDefaultUnitPanelIconBySeed(IconSeed);
		}

		if (!Data.Portrait)
		{
			Data.Portrait = LoadDefaultUnitAvatarBySeed(IconSeed);
		}
	}

	void ApplyMassProtocolToUnitData(FRTSUnitData& Data, const FRTSMassUnitTypeProtocol* Protocol, int32 SubTypeIndex)
	{
		if (!Protocol)
		{
			return;
		}

		Data.TypeKey = GetMassProtocolTypeKey(Protocol, SubTypeIndex);
		Data.UnitTypeTag = Protocol->UnitTypeTag;
		const FString ProtocolRole = Protocol->Role.TrimStartAndEnd();
		if (!ProtocolRole.IsEmpty())
		{
			Data.Role = ProtocolRole;
		}
		Data.AnnouncerId = Protocol->AnnouncerId;
		Data.SelectionSound = Protocol->SelectionSound;
		Data.ConfirmationSound = Protocol->ConfirmationSound;
		Data.CommandGrid = Protocol->CommandGrid;

		if (UTexture2D* ProtocolIcon = LoadConfiguredTexture(Protocol->Icon))
		{
			Data.Icon = ProtocolIcon;
		}

		if (UTexture2D* ProtocolPortrait = LoadConfiguredTexture(Protocol->Portrait))
		{
			Data.Portrait = ProtocolPortrait;
		}
	}

	ERTSCommandTargetType ResolveCommandSlotTargetType(const FRTSMassUnitCommandSlotDefinition& Slot, const FGameplayTag& CommandTag)
	{
		if (Slot.TargetType != ERTSCommandTargetType::Instant)
		{
			return Slot.TargetType;
		}

		const FName TagName = CommandTag.GetTagName();
		if (TagName == FName(TEXT("RTS.Command.Move")) || TagName == FName(TEXT("RTS.Command.Patrol")))
		{
			return ERTSCommandTargetType::Location;
		}

		if (TagName == FName(TEXT("RTS.Command.Attack")))
		{
			return ERTSCommandTargetType::LocationOrTarget;
		}

		if (TagName.ToString().StartsWith(TEXT("RTS.Command.Build.")))
		{
			return ERTSCommandTargetType::Location;
		}

		return ERTSCommandTargetType::Instant;
	}

	URTSCommandButton* CreateDefaultCommandButtonForTag(UObject* Outer, const FGameplayTag& CommandTag)
	{
		const FName TagName = CommandTag.GetTagName();
		if (TagName == FName(TEXT("RTS.Command.Move")))
		{
			return NewObject<URTSCmd_Move>(Outer);
		}
		if (TagName == FName(TEXT("RTS.Command.Attack")))
		{
			return NewObject<URTSCmd_Attack>(Outer);
		}
		if (TagName == FName(TEXT("RTS.Command.Stop")))
		{
			return NewObject<URTSCmd_Stop>(Outer);
		}
		if (TagName == FName(TEXT("RTS.Command.Hold")))
		{
			return NewObject<URTSCmd_HoldPosition>(Outer);
		}
		if (TagName == FName(TEXT("RTS.Command.Patrol")))
		{
			return NewObject<URTSCmd_Patrol>(Outer);
		}

		URTSCommandButton* Button = NewObject<URTSCommandButton>(Outer);
		Button->CommandTag = CommandTag;
		Button->TargetType = TagName.ToString().StartsWith(TEXT("RTS.Command.Build."))
			? ERTSCommandTargetType::Location
			: ERTSCommandTargetType::Instant;

		FString Label = TagName.ToString();
		Label.RemoveFromStart(TEXT("RTS.Command."));
		Label.ReplaceInline(TEXT("."), TEXT(" "));
		Button->DisplayName = FText::FromString(Label.IsEmpty() ? TEXT("Command") : Label);
		Button->Description = FText::FromString(FString::Printf(TEXT("Execute %s."), *Button->DisplayName.ToString()));
		return Button;
	}

	void AddDefaultUnitCommands(URTSCommandGridAsset* Grid)
	{
		if (!Grid)
		{
			return;
		}

		Grid->Buttons.Add(NewObject<URTSCmd_Move>(Grid));
		Grid->Buttons.Add(NewObject<URTSCmd_Attack>(Grid));
		Grid->Buttons.Add(NewObject<URTSCmd_Stop>(Grid));
		Grid->Buttons.Add(NewObject<URTSCmd_HoldPosition>(Grid));
		Grid->Buttons.Add(NewObject<URTSCmd_Patrol>(Grid));
	}

	void RemoveCommandAtSlot(URTSCommandGridAsset* Grid, int32 SlotIndex)
	{
		if (!Grid)
		{
			return;
		}

		Grid->Buttons.RemoveAll([SlotIndex](const TObjectPtr<URTSCommandButton>& Button)
		{
			return Button && Button->PreferredIndex == SlotIndex;
		});
	}

	void ApplyCommandSlotPresentation(URTSCommandButton* Button, const FRTSMassUnitCommandSlotDefinition& Slot, const FGameplayTag& CommandTag)
	{
		if (!Button)
		{
			return;
		}

		Button->CommandTag = CommandTag;
		Button->TargetType = ResolveCommandSlotTargetType(Slot, CommandTag);
		Button->PreferredIndex = Slot.SlotIndex;
		Button->bHideIfUnavailable = Slot.bHideIfUnavailable;

		if (!Slot.DisplayName.TrimStartAndEnd().IsEmpty())
		{
			Button->DisplayName = FText::FromString(Slot.DisplayName);
		}

		if (!Slot.Description.TrimStartAndEnd().IsEmpty())
		{
			Button->Description = FText::FromString(Slot.Description);
		}

		if (!Slot.Hotkey.IsNone())
		{
			Button->Hotkey = FKey(Slot.Hotkey);
		}

		if (UTexture2D* SlotIcon = LoadConfiguredTexture(Slot.Icon))
		{
			Button->Icon = SlotIcon;
		}
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

FString URTSSelectionSubsystem::GetMassSubtypeDisplayName(int32 SubTypeIndex) const
{
	const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
	if (const FRTSMassUnitTypeProtocol* Protocol = FindMassUnitTypeProtocolByIndex(Settings, SubTypeIndex))
	{
		const FString ProtocolName = Protocol->DisplayName.TrimStartAndEnd();
		if (!ProtocolName.IsEmpty())
		{
			return ProtocolName;
		}
	}

	if (Settings && Settings->MassUnitAvatars.IsValidIndex(SubTypeIndex))
	{
		const FString ConfiguredName = Settings->MassUnitAvatars[SubTypeIndex].DisplayName.TrimStartAndEnd();
		if (!ConfiguredName.IsEmpty())
		{
			return ConfiguredName;
		}
	}

	return GetDefaultMassSubtypeDisplayName(SubTypeIndex);
}

UTexture2D* URTSSelectionSubsystem::GetMassSubtypeUnitPanelIcon(int32 SubTypeIndex) const
{
	const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
	if (const FRTSMassUnitTypeProtocol* Protocol = FindMassUnitTypeProtocolByIndex(Settings, SubTypeIndex))
	{
		if (UTexture2D* ProtocolIcon = LoadConfiguredTexture(Protocol->Icon))
		{
			return ProtocolIcon;
		}
	}

	return LoadDefaultUnitPanelIconBySeed(static_cast<uint32>(SubTypeIndex));
}

UTexture2D* URTSSelectionSubsystem::GetMassSubtypeUnitAvatar(int32 SubTypeIndex) const
{
	const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
	if (const FRTSMassUnitTypeProtocol* Protocol = FindMassUnitTypeProtocolByIndex(Settings, SubTypeIndex))
	{
		if (UTexture2D* ProtocolPortrait = LoadConfiguredTexture(Protocol->Portrait))
		{
			return ProtocolPortrait;
		}
	}

	if (Settings && Settings->MassUnitAvatars.IsValidIndex(SubTypeIndex))
	{
		if (UTexture2D* LegacyAvatar = LoadConfiguredTexture(Settings->MassUnitAvatars[SubTypeIndex].Avatar))
		{
			return LegacyAvatar;
		}
	}

	return LoadDefaultUnitAvatarBySeed(static_cast<uint32>(SubTypeIndex));
}

void URTSSelectionSubsystem::SetSelectedUnits(const TArray<AActor*>& InActors, const TArray<FEntityHandle>& InEntities, ERTSSelectionModifier Modifier)
{
    TArray<AActor*> FinalActors;
    TArray<FEntityHandle> FinalEntities;

	for (AActor* Actor : InActors)
	{
		if (IsValid(Actor))
		{
			FinalActors.AddUnique(Actor);
		}
	}

	for (const FEntityHandle& Handle : InEntities)
	{
		if (Handle.Index != 0)
		{
			FinalEntities.AddUnique(Handle);
		}
	}

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

	const FRTSSelectionView View = BuildSelectionView();
	BroadcastSelectionViewAndGrid(View);

    UE_LOG(LogORTSSelection, Log, TEXT("Selection: Modifier=%d Actors=%d Entities=%d ActiveKey=%s"),
        (int32)Modifier, SelectedActors.Num(), SelectedEntities.Num(), *View.ActiveGroupKey);
}

FRTSSelectionView URTSSelectionSubsystem::BuildSelectionView()
{
	FRTSSelectionView View;
	int32 TotalCount = SelectedActors.Num() + SelectedEntities.Num();
	const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
	const int32 SummaryThreshold = Settings
		? FMath::Max(1, Settings->SelectionSummaryThreshold)
		: 16;

	if (TotalCount == 0)
	{
		View.Mode = ERTSSelectionMode::Empty;
	}
	else if (TotalCount == 1)
	{
		View.Mode = ERTSSelectionMode::Single;
		if (SelectedActors.Num() > 0) View.SingleUnit = CreateUnitDataFromActor(SelectedActors[0]);
		else View.SingleUnit = CreateUnitDataFromEntity(SelectedEntities[0]);
		View.Items.Add(View.SingleUnit);
	}
	else if (TotalCount <= SummaryThreshold)
	{
		View.Mode = ERTSSelectionMode::List;
		for (AActor* Actor : SelectedActors) View.Items.Add(CreateUnitDataFromActor(Actor));
		for (const FEntityHandle& Handle : SelectedEntities) View.Items.Add(CreateUnitDataFromEntity(Handle));
		View.Items.Sort([](const FRTSUnitData& A, const FRTSUnitData& B)
		{
			const int32 NameCompare = A.Name.Compare(B.Name);
			return NameCompare == 0 ? GetSelectionUnitGroupKey(A) < GetSelectionUnitGroupKey(B) : NameCompare < 0;
		});
	}
	else
	{
		View.Mode = ERTSSelectionMode::Summary;
		TMap<FString, FRTSUnitData> GroupMap;

		for (AActor* Actor : SelectedActors)
		{
			FRTSUnitData Data = CreateUnitDataFromActor(Actor);
			AddOrUpdateSummaryGroup(GroupMap, Data);
		}

		for (const FEntityHandle& Handle : SelectedEntities)
		{
			FRTSUnitData Data = CreateUnitDataFromEntity(Handle);
			AddOrUpdateSummaryGroup(GroupMap, Data);
		}

		for (auto& Pair : GroupMap) View.Items.Add(Pair.Value);
		View.Items.Sort([](const FRTSUnitData& A, const FRTSUnitData& B)
		{
			const int32 NameCompare = A.Name.Compare(B.Name);
			return NameCompare == 0 ? GetSelectionUnitGroupKey(A) < GetSelectionUnitGroupKey(B) : NameCompare < 0;
		});
	}

	// --- Tab Cycling ---
	const FString PreviousActiveKey = AvailableGroupKeys.IsValidIndex(CurrentGroupIndex)
		? AvailableGroupKeys[CurrentGroupIndex]
		: FString();

	AvailableGroupKeys.Reset();
	for (const auto& Item : View.Items)
	{
		AvailableGroupKeys.AddUnique(GetSelectionUnitGroupKey(Item));
	}
	AvailableGroupKeys.Sort();

	if (!PreviousActiveKey.IsEmpty())
	{
		const int32 PreservedIndex = AvailableGroupKeys.IndexOfByKey(PreviousActiveKey);
		if (PreservedIndex != INDEX_NONE)
		{
			CurrentGroupIndex = PreservedIndex;
		}
	}

	if (CurrentGroupIndex >= AvailableGroupKeys.Num() || CurrentGroupIndex < 0) CurrentGroupIndex = 0;
	if (AvailableGroupKeys.IsValidIndex(CurrentGroupIndex)) View.ActiveGroupKey = AvailableGroupKeys[CurrentGroupIndex];

	return View;
}

void URTSSelectionSubsystem::AddOrUpdateSummaryGroup(TMap<FString, FRTSUnitData>& GroupMap, const FRTSUnitData& Data)
{
	const FString GroupKey = GetSelectionUnitGroupKey(Data);
	if (FRTSUnitData* ExistingGroup = GroupMap.Find(GroupKey))
	{
		ExistingGroup->Count++;
		return;
	}

	FRTSUnitData NewGroup = Data;
	NewGroup.GroupKey = GroupKey;
	NewGroup.Count = 1;
	GroupMap.Add(GroupKey, NewGroup);
}

FRTSExternalMassCommandGridResolver& URTSSelectionSubsystem::OnResolveMassCommandGrid()
{
	static FRTSExternalMassCommandGridResolver Resolver;
	return Resolver;
}

FRTSExternalMassLocationCommandHandler& URTSSelectionSubsystem::OnHandleMassLocationCommand()
{
	static FRTSExternalMassLocationCommandHandler Handler;
	return Handler;
}

FRTSExternalMassTargetCommandHandler& URTSSelectionSubsystem::OnHandleMassTargetCommand()
{
	static FRTSExternalMassTargetCommandHandler Handler;
	return Handler;
}

void URTSSelectionSubsystem::BroadcastSelectionViewAndGrid(const FRTSSelectionView& View)
{
	OnSelectionChanged.Broadcast(View);

    // --- Grid Synchronization ---
    // ActiveGroupKey is the stable group id; UI may display a friendlier unit name.
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

		// 路径B: 外部插件可先为纯 Mass 实体提供命令面板。
		if (!NewGrid)
		{
			OnResolveMassCommandGrid().Broadcast(this, ActiveKey, View, NewGrid);
		}

		if (NewGrid)
		{
			OnCommandNavigationRequested.Broadcast(NewGrid);
			UE_LOG(LogORTSSelection, Log, TEXT("Selection: Grid sync ActiveKey=%s Grid=>%s"),
				*ActiveKey, *NewGrid->GetName());
			return;
		}

		// 路径C: Mass Entity 组 —— 用 type protocol 推送专属/稀疏技能面板。
		bool bMassProtocolHandledGrid = false;
		if (!NewGrid)
		{
			bMassProtocolHandledGrid = ResolveMassProtocolCommandGrid(ActiveKey, NewGrid);
		}

		if (bMassProtocolHandledGrid)
		{
			OnCommandNavigationRequested.Broadcast(NewGrid);
			UE_LOG(LogORTSSelection, Log, TEXT("Selection: Grid sync ActiveKey=%s Grid=>%s"),
				*ActiveKey, NewGrid ? *NewGrid->GetName() : TEXT("NULL"));
			return;
		}
    }

    // 路径D: 兜底默认 Grid（士兵移动/攻击/停止）
    if (!NewGrid && !DefaultEntityGrid.IsNull() && (SelectedActors.Num() > 0 || SelectedEntities.Num() > 0))
    {
        NewGrid = DefaultEntityGrid.LoadSynchronous();
    }

    OnCommandNavigationRequested.Broadcast(NewGrid);
    UE_LOG(LogORTSSelection, Log, TEXT("Selection: Grid sync ActiveKey=%s Grid=>%s"),
        *ActiveKey, NewGrid ? *NewGrid->GetName() : TEXT("NULL"));
}

bool URTSSelectionSubsystem::ResolveMassProtocolCommandGrid(const FString& ActiveKey, URTSCommandGridAsset*& OutGrid)
{
	OutGrid = nullptr;

	const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
	int32 SubTypeIndex = INDEX_NONE;
	const FRTSMassUnitTypeProtocol* Protocol = ResolveMassUnitTypeProtocol(Settings, ActiveKey, SubTypeIndex);
	if (!Protocol)
	{
		return false;
	}

	if (!Protocol->CommandGrid.IsNull())
	{
		if (URTSCommandGridAsset* LoadedGrid = Protocol->CommandGrid.LoadSynchronous())
		{
			if (LoadedGrid->GetAllButtons().Num() > 0)
			{
				OutGrid = LoadedGrid;
				return true;
			}
		}
	}

	if (Protocol->CommandSlots.Num() > 0)
	{
		if (TObjectPtr<URTSCommandGridAsset>* CachedGrid = MassProtocolGridCache.Find(SubTypeIndex))
		{
			OutGrid = CachedGrid->Get();
			return true;
		}

		const FName GridName(*FString::Printf(TEXT("MassType_%02d_CommandGrid"), SubTypeIndex));
		URTSCommandGridAsset* TransientGrid = NewObject<URTSCommandGridAsset>(this, GridName);
		if (Protocol->bUseDefaultCommandGrid)
		{
			AddDefaultUnitCommands(TransientGrid);
		}

		for (const FRTSMassUnitCommandSlotDefinition& Slot : Protocol->CommandSlots)
		{
			FGameplayTag ResolvedCommandTag = Slot.CommandTag;
			if (!ResolvedCommandTag.IsValid() && !Slot.CommandTagName.IsNone())
			{
				ResolvedCommandTag = FGameplayTag::RequestGameplayTag(Slot.CommandTagName, false);
			}

			if (!ResolvedCommandTag.IsValid() || Slot.SlotIndex < 0 || Slot.SlotIndex > 14)
			{
				continue;
			}

			RemoveCommandAtSlot(TransientGrid, Slot.SlotIndex);

			URTSCommandButton* Button = CreateDefaultCommandButtonForTag(TransientGrid, ResolvedCommandTag);
			ApplyCommandSlotPresentation(Button, Slot, ResolvedCommandTag);
			TransientGrid->Buttons.Add(Button);
		}

		if (TransientGrid->Buttons.Num() > 0)
		{
			MassProtocolGridCache.Add(SubTypeIndex, TransientGrid);
			OutGrid = TransientGrid;
			return true;
		}
	}

	if (Protocol->bUseDefaultCommandGrid)
	{
		if (TObjectPtr<URTSCommandGridAsset>* CachedGrid = MassProtocolGridCache.Find(SubTypeIndex))
		{
			OutGrid = CachedGrid->Get();
			return true;
		}

		const FName GridName(*FString::Printf(TEXT("MassType_%02d_DefaultCommandGrid"), SubTypeIndex));
		URTSCommandGridAsset* TransientGrid = NewObject<URTSCommandGridAsset>(this, GridName);
		AddDefaultUnitCommands(TransientGrid);
		MassProtocolGridCache.Add(SubTypeIndex, TransientGrid);
		OutGrid = TransientGrid;
		return true;
	}

	return false;
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

	const FRTSSelectionView View = BuildSelectionView();
	BroadcastSelectionViewAndGrid(View);
}

void URTSSelectionSubsystem::RemoveUnit(const FRTSUnitData& UnitData)
{
	TArray<AActor*> ActorsToRemove;
	TArray<FEntityHandle> EntitiesToRemove;

	if (UnitData.Count <= 1 && UnitData.ActorPtr) ActorsToRemove.Add(UnitData.ActorPtr);
	else if (UnitData.Count <= 1 && UnitData.EntityHandle.Index != 0) EntitiesToRemove.Add(UnitData.EntityHandle);
	else
	{
		const FString UnitKey = GetSelectionUnitGroupKey(UnitData);
		for (AActor* Act : SelectedActors) if (Act && GetActorGroupKey(Act) == UnitKey) ActorsToRemove.Add(Act);
		for (const FEntityHandle& Handle : SelectedEntities)
		{
			const FRTSUnitData Data = CreateUnitDataFromEntity(Handle);
			if (GetSelectionUnitGroupKey(Data) == UnitKey)
			{
				EntitiesToRemove.Add(Handle);
			}
		}
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
        if (GetSelectionUnitGroupKey(Data) == GroupKey) NewEntities.Add(Handle);
    }

	SetSelectedUnits(NewActors, NewEntities, ERTSSelectionModifier::Replace);
}

FRTSUnitData URTSSelectionSubsystem::CreateUnitDataFromActor(AActor* Actor) const
{
	FRTSUnitData Data;
	if (Actor)
	{
		Data.GroupKey = GetActorGroupKey(Actor);
		Data.TypeKey = Data.GroupKey;
		Data.Name = Data.GroupKey;
		Data.ActorPtr = Actor;
		Data.bIsMassEntity = false;
		
		if (auto Selectable = Actor->FindComponentByClass<URTSSelectable>())
		{
			Data.Icon = Selectable->Icon;
			Data.Portrait = Selectable->Avatar;
			Data.Health = Selectable->Health;
			Data.MaxHealth = Selectable->MaxHealth;
			Data.Energy = Selectable->Energy;
			Data.MaxEnergy = Selectable->MaxEnergy;
			Data.Shield = Selectable->Shield;
			Data.MaxShield = Selectable->MaxShield;
		}

		if (!Data.Icon)
		{
			Data.Icon = LoadDefaultUnitPanelIconBySeed(GetTypeHash(Data.Name));
		}
		EnsureSelectionDataDefaults(Data, INDEX_NONE, GetTypeHash(Data.Name));
	}
	return Data;
}

FRTSUnitData URTSSelectionSubsystem::CreateUnitDataFromEntity(const FEntityHandle& Handle) const
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
					const int32 SubTypeIndex = SubFrag->Index;
					const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
					const FRTSMassUnitTypeProtocol* Protocol = FindMassUnitTypeProtocolByIndex(Settings, SubTypeIndex);
					Data.SubTypeIndex = SubTypeIndex;
					Data.TypeKey = GetMassProtocolTypeKey(Protocol, SubTypeIndex);
					Data.GroupKey = Data.TypeKey;
                    Data.Name = GetMassSubtypeDisplayName(SubTypeIndex);
					Data.Icon = GetMassSubtypeUnitPanelIcon(SubTypeIndex);
					Data.Portrait = GetMassSubtypeUnitAvatar(SubTypeIndex);
					ApplyMassProtocolToUnitData(Data, Protocol, SubTypeIndex);

					if (const FHealth* Health = EM.GetFragmentDataPtr<FHealth>(NativeHandle))
					{
						Data.Health = Health->Current;
						Data.MaxHealth = Health->Maximum;
					}

					EnsureSelectionDataDefaults(Data, SubTypeIndex, static_cast<uint32>(SubTypeIndex));
                    return Data;
                }
            }
        }
    }

    Data.Name = TEXT("Mass Unit");
	Data.GroupKey = FString::Printf(TEXT("MassUnit.Entity.%d"), Handle.Index);
	Data.TypeKey = Data.GroupKey;
	Data.Icon = LoadDefaultUnitPanelIconBySeed(static_cast<uint32>(Handle.Index));
	EnsureSelectionDataDefaults(Data, INDEX_NONE, static_cast<uint32>(Handle.Index));
	return Data;
}


void URTSSelectionSubsystem::IssueCommand(FGameplayTag CommandTag)
{
    UE_LOG(LogTemp, Log, TEXT("RTSSelectionSubsystem: Command %s Issued to Current Selection."), *CommandTag.ToString());

    if (SelectedEntities.Num() > 0)
    {
        if (ULocalPlayer* LP = GetLocalPlayer())
        {
            if (URTSCommandSubsystem* SignalHub = LP->GetSubsystem<URTSCommandSubsystem>())
            {
                SignalHub->IssueCommand(CommandTag, nullptr);
            }
        }
    }

	for (AActor* Actor : SelectedActors)
	{
		if (Actor && Actor->Implements<URTSCommandInterface>())
		{
			IRTSCommandInterface::Execute_ExecuteCommand(Actor, CommandTag);
		}
	}

    RequestCommandRefresh();
}

void URTSSelectionSubsystem::IssueCommandWithLocation(FGameplayTag CommandTag, FVector Location)
{
    UE_LOG(LogTemp, Log, TEXT("RTSSelectionSubsystem: Command %s Issued with Location %s"), *CommandTag.ToString(), *Location.ToString());

    if (SelectedEntities.Num() > 0)
    {
		bool bHandledByExternalMassSystem = false;
		const FRTSSelectionView View = BuildSelectionView();
		OnHandleMassLocationCommand().Broadcast(this, CommandTag, Location, View, bHandledByExternalMassSystem);

        if (ULocalPlayer* LP = GetLocalPlayer())
        {
            if (!bHandledByExternalMassSystem)
            {
                if (URTSCommandSubsystem* SignalHub = LP->GetSubsystem<URTSCommandSubsystem>())
                {
                    SignalHub->IssueCommandWithLocation(CommandTag, Location);
                }
            }
        }
    }

	for (AActor* Actor : SelectedActors)
	{
		if (Actor && Actor->Implements<URTSCommandInterface>())
		{
			IRTSCommandInterface::Execute_ExecuteCommandWithLocation(Actor, CommandTag, Location);
		}
	}

    RequestCommandRefresh();
}

void URTSSelectionSubsystem::IssueCommandWithTarget(FGameplayTag CommandTag, AActor* TargetActor)
{
    UE_LOG(LogTemp, Log, TEXT("RTSSelectionSubsystem: Command %s Issued with TargetActor %s"), *CommandTag.ToString(), TargetActor ? *TargetActor->GetName() : TEXT("NULL"));

    if (SelectedEntities.Num() > 0)
    {
		bool bHandledByExternalMassSystem = false;
		const FRTSSelectionView View = BuildSelectionView();
		OnHandleMassTargetCommand().Broadcast(this, CommandTag, TargetActor, View, bHandledByExternalMassSystem);

        if (ULocalPlayer* LP = GetLocalPlayer())
        {
            if (!bHandledByExternalMassSystem)
            {
                if (URTSCommandSubsystem* SignalHub = LP->GetSubsystem<URTSCommandSubsystem>())
                {
                    SignalHub->IssueCommandWithTarget(CommandTag, TargetActor);
                }
            }
        }
    }

	for (AActor* Actor : SelectedActors)
	{
		if (Actor && Actor->Implements<URTSCommandInterface>())
		{
			IRTSCommandInterface::Execute_ExecuteCommandWithTarget(Actor, CommandTag, TargetActor);
		}
	}

    RequestCommandRefresh();
}

FString URTSSelectionSubsystem::GetActiveGroupKey() const
{
	if (AvailableGroupKeys.IsValidIndex(CurrentGroupIndex))
	{
		return AvailableGroupKeys[CurrentGroupIndex];
	}

	return FString();
}

TArray<FEntityHandle> URTSSelectionSubsystem::GetActiveMassEntities() const
{
	const FString ActiveKey = GetActiveGroupKey();
	if (ActiveKey.IsEmpty())
	{
		return SelectedEntities;
	}

	TArray<FEntityHandle> Result;
	for (const FEntityHandle& Handle : SelectedEntities)
	{
		FRTSUnitData Data = CreateUnitDataFromEntity(Handle);
		if (GetSelectionUnitGroupKey(Data) == ActiveKey)
		{
			Result.Add(Handle);
		}
	}

	return Result.Num() > 0 ? Result : SelectedEntities;
}

AActor* URTSSelectionSubsystem::GetActiveActor() const
{
    if (SelectedActors.Num() == 0) return nullptr;
    const FString ActiveKey = GetActiveGroupKey();
    if (!ActiveKey.IsEmpty())
    {
        for (AActor* Actor : SelectedActors)
        {
            if (Actor && GetActorGroupKey(Actor) == ActiveKey) return Actor;
        }
    }
    return SelectedActors[0];
}
