#pragma once

#include "CoreMinimal.h"
#include "MassAPIStructs.h"
#include "Tasks/MassBattleBPTaskAgentsMoveTo.h"

class UWorld;

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
};
