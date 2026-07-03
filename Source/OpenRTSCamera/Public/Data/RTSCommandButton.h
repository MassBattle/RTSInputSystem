// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "InputCoreTypes.h"
#include "RTSCommandButton.generated.h"

UENUM(BlueprintType)
enum class ERTSCommandTargetType : uint8
{
	Instant UMETA(DisplayName = "Instant"),
	Location UMETA(DisplayName = "Location"),
	TargetActor UMETA(DisplayName = "Target Actor"),
	LocationOrTarget UMETA(DisplayName = "Location or Target")
};

/**
 * Represents a single button in the RTS Command Card (UI).
 * This asset is pure data and references the logical command via GameplayTag.
 */
UCLASS(BlueprintType)
class OPENRTSCAMERA_API URTSCommandButton : public UDataAsset
{
	GENERATED_BODY()

public:

	// The logical command to execute when clicked (e.g. RTS.Command.Move)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Command")
	FGameplayTag CommandTag;

	// Does this command require a target location or actor?
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Command")
	ERTSCommandTargetType TargetType = ERTSCommandTargetType::Instant;

	// The icon to display in the grid
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	TObjectPtr<UTexture2D> Icon;

	// The tooltip title
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FText DisplayName;

	// The detailed tooltip description
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	FText Description;

	// The preferred index in the 15-grid (0-14). (-1 = Auto)
	// Row 1: 0-4 (Basic Commands)
	// Row 2: 5-9 (Advanced/Stop/Hold)
	// Row 3: 10-14 (Abilities)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI", meta = (ClampMin = "-1", ClampMax = "14"))
	int32 PreferredIndex = -1;

	// The hotkey for this button
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Input")
	FKey Hotkey;

	// Should this button be hidden if the command is unavailable?
	// If false, it will be shown as disabled (greyed out).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "UI")
	bool bHideIfUnavailable = false;

    // Base cooldown in seconds
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameLogic")
    float DefaultCooldown = 0.0f;

    // Does this command support auto-cast toggle?
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GameLogic")
    bool bAllowAutoCast = false;

    // --- Costs ---
    // Minerals / Low Value Resource
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
    int32 LowValueCost = 0;

    // Gas / High Value Resource
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Cost")
    int32 HighValueCost = 0;

    // --- Fluff / Info ---
    // E.g. "Production Building", "Infantry", "Vehicle"
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Info")
    FText Role;

    // Prerequisite Tags (e.g. "Tech.TechCenter")
    // The Tag Name (GetTagName) will be displayed in the Tooltip.
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Logic")
    TArray<FGameplayTag> Requirements;

    /**
     * Executes the logic associated with this button.
     * @param Executor The Actor that is performing the command.
     */
    UFUNCTION(BlueprintNativeEvent, Category = "RTS Command")
    void Execute(AActor* Executor);
    virtual void Execute_Implementation(AActor* Executor);
};
