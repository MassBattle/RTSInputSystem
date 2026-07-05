// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "MassEntityTypes.h"
#include "MassAPIStructs.h"
#include "UObject/SoftObjectPtr.h"
#include "RTSSelectionStructs.generated.h"

class UTexture2D;
class USoundBase;
class URTSCommandGridAsset;

UENUM(BlueprintType)
enum class ERTSSelectionMode : uint8
{
	Empty   = 0 UMETA(DisplayName = "No Selection"),
	Single  = 1 UMETA(DisplayName = "Single Unit"),
	List    = 2 UMETA(DisplayName = "Unit List"),
	Summary = 3 UMETA(DisplayName = "Group Summary")
};

UENUM(BlueprintType)
enum class ERTSSelectionModifier : uint8
{
	Replace     UMETA(DisplayName = "Replace Selection"),
	Add         UMETA(DisplayName = "Add to Selection"),
	Remove      UMETA(DisplayName = "Remove from Selection")
};

/**
 * Unified data structure representing a single selectable unit OR a group summary.
 */
USTRUCT(BlueprintType)
struct FRTSUnitData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	FString Name;

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	FString GroupKey;

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	FString TypeKey;

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	int32 SubTypeIndex = INDEX_NONE;

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	FGameplayTag UnitTypeTag;

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	FString Role;

	/** Small unit icon used inside the RTS unit panel, selection grid, and summary cells. */
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	UTexture2D* Icon = nullptr;

	/** Larger avatar/portrait for the left-side single-selection detail panel. Kept separate from Icon. */
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	UTexture2D* Portrait = nullptr;

	/** Lightweight announcer cue key for UI/audio systems. */
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	FName AnnouncerId;

	/** Optional selected voice asset. Soft so protocol data can be pushed without forcing all audio loaded. */
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	TSoftObjectPtr<USoundBase> SelectionSound;

	/** Optional command acknowledgement voice asset. */
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	TSoftObjectPtr<USoundBase> ConfirmationSound;

	/** Command grid associated with this type, when one is authored as an asset. */
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	TSoftObjectPtr<URTSCommandGridAsset> CommandGrid;

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	int32 Count = 1; // 1 for individual unit, >1 for group summary

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	float Health = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	float MaxHealth = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	float Energy = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	float MaxEnergy = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	float Shield = 0.0f;
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	float MaxShield = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	bool bIsMassEntity = false;

	// Optional: Raw pointers/handles if UI needs to command them back
	// Only valid if Count == 1
	UPROPERTY()
	AActor* ActorPtr = nullptr;

	UPROPERTY()
	FEntityHandle EntityHandle;


	// Default constructor for "Empty/Unknown" state
	FRTSUnitData()
	{
		Name = TEXT("Unknown");
		GroupKey = TEXT("Unknown");
	}
};

/**
 * The snapshot sent to the UI.
 */
USTRUCT(BlueprintType)
struct FRTSSelectionView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	ERTSSelectionMode Mode = ERTSSelectionMode::Empty;


	// Used only when Mode == Single. Contains detailed info.
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	FRTSUnitData SingleUnit;

	// Used when Mode == List (individual items, Count=1) or Summary (grouped items, Count>1).
	// For Single, this may contain the single unit for consumers that need the current item/style.
	// For Empty, this must be empty.
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	TArray<FRTSUnitData> Items;

	// The key of the currently active sub-group (e.g. "Marine")
	// Used for highlighting and tab-cycling
	UPROPERTY(BlueprintReadOnly, Category = "RTS Selection")
	FString ActiveGroupKey;
};
