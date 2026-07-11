// Copyright 2024 Winy unq All Rights Reserved.

#include "RTSCommandSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "MassAPIFuncLib.h"
#include "MassBattleStructs.h"
#include "Tasks/MassBattleBPTaskAgentsMoveTo.h"
#include "Tasks/MassBattleBPTaskAgentsChaseAttack.h"
#include "FuncLibs/MassBattleFuncLib.h"
#include "Components/MassBattleAgentComponent.h"
#include "GameplayTagsManager.h"
#include "RTSSelectionSubsystem.h"

namespace
{
	const FName MoveTagName(TEXT("RTS.Command.Move"));
	const FName AttackTagName(TEXT("RTS.Command.Attack"));
	const FName StopTagName(TEXT("RTS.Command.Stop"));
	const FName HoldTagName(TEXT("RTS.Command.Hold"));
	const FName PatrolTagName(TEXT("RTS.Command.Patrol"));

	FGameplayTag GetCommandTag(const FName TagName)
	{
		return FGameplayTag::RequestGameplayTag(TagName, false);
	}
}

void URTSCommandSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	UGameplayTagsManager::Get().AddNativeGameplayTag(MoveTagName, TEXT("Default RTS unit move command"));
	UGameplayTagsManager::Get().AddNativeGameplayTag(AttackTagName, TEXT("Default RTS unit attack command"));
	UGameplayTagsManager::Get().AddNativeGameplayTag(StopTagName, TEXT("Default RTS unit stop command"));
	UGameplayTagsManager::Get().AddNativeGameplayTag(HoldTagName, TEXT("Default RTS unit hold command"));
	UGameplayTagsManager::Get().AddNativeGameplayTag(PatrolTagName, TEXT("Default RTS unit patrol command"));
}

void URTSCommandSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void URTSCommandSubsystem::IssueCommand(FGameplayTag Tag, AActor* /*Context*/)
{
	ExecuteCommand(Tag, GetSelectedMassEntities(), nullptr, nullptr);
}

void URTSCommandSubsystem::IssueCommandWithLocation(FGameplayTag Tag, const FVector& Location)
{
	ExecuteCommand(Tag, GetSelectedMassEntities(), &Location, nullptr);
}

void URTSCommandSubsystem::IssueCommandWithTarget(FGameplayTag Tag, AActor* TargetActor)
{
	ExecuteCommand(Tag, GetSelectedMassEntities(), nullptr, TargetActor);
}

TArray<FEntityHandle> URTSCommandSubsystem::GetSelectedMassEntities() const
{
	if (ULocalPlayer* LP = const_cast<URTSCommandSubsystem*>(this)->GetLocalPlayer())
	{
		if (URTSSelectionSubsystem* Selection = LP->GetSubsystem<URTSSelectionSubsystem>())
		{
			return Selection->GetSelectedEntities();
		}
	}

	FMassBattleQuery Query;
	Query.BattleAllFlagsList.Reset();
	Query.BattleAllFlagsList.Add(EBattleFlags::Selected);

	const FEntityQuery EntityQuery = Query.ToEntityQuery();
	return UMassAPIFuncLib::GetMatchingEntities(this, EntityQuery);
}

void URTSCommandSubsystem::ExecuteCommand(
	FGameplayTag Tag,
	const TArray<FEntityHandle>& SelectedEntities,
	const FVector* Location,
	AActor* TargetActor
)
{
	if (!Tag.IsValid() || SelectedEntities.Num() == 0)
	{
		return;
	}

	const FGameplayTag MoveTag = GetCommandTag(MoveTagName);
	const FGameplayTag AttackTag = GetCommandTag(AttackTagName);
	const FGameplayTag StopTag = GetCommandTag(StopTagName);
	const FGameplayTag HoldTag = GetCommandTag(HoldTagName);
	const FGameplayTag PatrolTag = GetCommandTag(PatrolTagName);

	if (Tag == MoveTag || Tag == PatrolTag)
	{
		if (!Location) return;
		IssueMoveTo(SelectedEntities, *Location, /*bCanInterrupt*/ false);
		return;
	}

	if (Tag == AttackTag)
	{
		if (TargetActor)
		{
			IssueAttackTarget(SelectedEntities, TargetActor);
			return;
		}

		if (Location)
		{
			IssueMoveTo(SelectedEntities, *Location, /*bCanInterrupt*/ true);
		}
		return;
	}

	if (Tag == StopTag || Tag == HoldTag)
	{
		UMassBattleFuncLib::StopAgentsAllMovement(this, SelectedEntities);
		return;
	}
}

void URTSCommandSubsystem::IssueMoveTo(const TArray<FEntityHandle>& SelectedEntities, const FVector& Location, bool bCanInterrupt)
{
	FMBMoveGoal Goal;
	Goal.GoalType = EMBMoveGoalType::Location;
	Goal.Locations.Add(Location);

	if (UMassBattleBPTaskAgentsMoveTo* MoveTask = UMassBattleBPTaskAgentsMoveTo::AgentsMoveTo(
		this,
		SelectedEntities,
		Goal,
		FMBMoveNavigation(),
		FMBMoveFailCondition(),
		bCanInterrupt
	))
	{
		MoveTask->Activate();
	}
}

void URTSCommandSubsystem::IssueAttackTarget(const TArray<FEntityHandle>& SelectedEntities, AActor* TargetActor)
{
	FEntityHandle TargetHandle;
	if (!TargetActor) return;

	if (const UMassBattleAgentComponent* TargetComp = TargetActor->FindComponentByClass<UMassBattleAgentComponent>())
	{
		TargetHandle = TargetComp->GetEntityHandle();
	}

	if (!UMassAPIFuncLib::IsValid(this, TargetHandle))
	{
		return;
	}

	if (UMassBattleBPTaskAgentsChaseAttack* ChaseTask = UMassBattleBPTaskAgentsChaseAttack::AgentsChaseAttack(
		this,
		SelectedEntities,
		TargetHandle,
		false,
		6.0f,
		100.0f
	))
	{
		ChaseTask->Activate();
	}
}
