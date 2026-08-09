#pragma once

#include "CoreMinimal.h"
#include "MassAPIStructs.h"
#include "Tasks/MassBattleBPTaskAgentsMoveTo.h"

class UWorld;
class UMassBattleAgentConfigDataAsset;

/** One native AgentsMoveTo batch sharing one navigation source and one goal. */
struct RTSINPUTSYSTEM_API FRTSMoveNavigationBatch
{
	TArray<FEntityHandle> Entities;
	FVector Goal = FVector::ZeroVector;
	FMBMoveNavigation Navigation;
};

// Command-time navigation extension. Implementations build navigation data;
// AgentsMoveTo remains the sole owner of entity task state and movement mode.
class RTSINPUTSYSTEM_API IRTSMoveNavigationProvider
{
public:
	virtual ~IRTSMoveNavigationProvider() = default;

	virtual bool BuildMoveNavigationBatches(
		const TArray<FEntityHandle>& Entities,
		const FVector& RequestedGoal,
		TArray<FRTSMoveNavigationBatch>& OutBatches) = 0;

	/**
	 * Projects a complete spawn formation into the unit's legal movement domain.
	 * Providers must return one canonical anchor so every member keeps its
	 * authored formation offset. The default keeps generic RTSInput behavior.
	 */
	virtual bool ResolveInitialSpawnLocation(
		const UMassBattleAgentConfigDataAsset* AgentConfig,
		const FVector& RequestedLocation,
		float FormationRadiusUU,
		FVector& OutLocation)
	{
		OutLocation = RequestedLocation;
		return true;
	}
};

/** World-scoped provider registry shared by player, production and AI orders. */
class RTSINPUTSYSTEM_API FRTSMoveNavigationProviderRegistry
{
public:
	static void Register(
		UWorld* World,
		IRTSMoveNavigationProvider* Provider);
	static void Unregister(
		UWorld* World,
		IRTSMoveNavigationProvider* Provider);
	static bool BuildBatches(
		UObject* WorldContext,
		const TArray<FEntityHandle>& Entities,
		const FVector& RequestedGoal,
		TArray<FRTSMoveNavigationBatch>& OutBatches);
	static bool ResolveInitialSpawnLocation(
		UObject* WorldContext,
		const UMassBattleAgentConfigDataAsset* AgentConfig,
		const FVector& RequestedLocation,
		float FormationRadiusUU,
		FVector& OutLocation);
};
