#include "RTSMoveNavigationProvider.h"

#include "Engine/Engine.h"
#include "Engine/World.h"

namespace
{
	TMap<TWeakObjectPtr<UWorld>, IRTSMoveNavigationProvider*> Providers;

	void PruneInvalidWorlds()
	{
		for (auto It = Providers.CreateIterator(); It; ++It)
		{
			if (!It.Key().IsValid())
			{
				It.RemoveCurrent();
			}
		}
	}
}

void FRTSMoveNavigationProviderRegistry::Register(
	UWorld* World,
	IRTSMoveNavigationProvider* Provider)
{
	check(IsInGameThread());
	if (!IsValid(World) || !Provider)
	{
		return;
	}
	PruneInvalidWorlds();
	Providers.Add(World, Provider);
}

void FRTSMoveNavigationProviderRegistry::Unregister(
	UWorld* World,
	IRTSMoveNavigationProvider* Provider)
{
	check(IsInGameThread());
	if (!World)
	{
		return;
	}
	if (IRTSMoveNavigationProvider** Existing = Providers.Find(World);
		Existing && *Existing == Provider)
	{
		Providers.Remove(World);
	}
}

bool FRTSMoveNavigationProviderRegistry::BuildBatches(
	UObject* WorldContext,
	const TArray<FEntityHandle>& Entities,
	const FVector& RequestedGoal,
	TArray<FRTSMoveNavigationBatch>& OutBatches)
{
	check(IsInGameThread());
	OutBatches.Reset();
	UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(
			WorldContext,
			EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World || Entities.IsEmpty())
	{
		return false;
	}

	PruneInvalidWorlds();
	if (IRTSMoveNavigationProvider** Provider = Providers.Find(World))
	{
		return *Provider
			&& (*Provider)->BuildMoveNavigationBatches(
				Entities,
				RequestedGoal,
				OutBatches);
	}

	// Generic RTSInput operation keeps MassBattle's stock Individual contract
	// when a project has not registered a navigation provider.
	FRTSMoveNavigationBatch& Batch = OutBatches.AddDefaulted_GetRef();
	Batch.Entities = Entities;
	Batch.Goal = RequestedGoal;
	Batch.Navigation.Mode = ENavMode::Individual;
	return true;
}

bool FRTSMoveNavigationProviderRegistry::ResolveInitialSpawnLocation(
	UObject* WorldContext,
	const UMassBattleAgentConfigDataAsset* AgentConfig,
	const FVector& RequestedLocation,
	const float FormationRadiusUU,
	FVector& OutLocation)
{
	check(IsInGameThread());
	OutLocation = RequestedLocation;
	UWorld* World = GEngine
		? GEngine->GetWorldFromContextObject(
			WorldContext,
			EGetWorldErrorMode::ReturnNull)
		: nullptr;
	if (!World || !AgentConfig)
	{
		return false;
	}

	PruneInvalidWorlds();
	if (IRTSMoveNavigationProvider** Provider = Providers.Find(World))
	{
		return *Provider
			&& (*Provider)->ResolveInitialSpawnLocation(
				AgentConfig,
				RequestedLocation,
				FMath::Max(0.0f, FormationRadiusUU),
				OutLocation);
	}

	// A project without a placement provider retains stock spawn semantics.
	return true;
}
