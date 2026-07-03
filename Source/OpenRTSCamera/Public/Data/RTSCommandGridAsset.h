// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "RTSCommandButton.h"
#include "RTSCommandGridAsset.generated.h"

/**
 * Defines a complete Command Card layout for a unit or group.
 * Similar to SC2's "Command Card".
 */
UCLASS(BlueprintType)
class OPENRTSCAMERA_API URTSCommandGridAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	// A unified collection of buttons defined in this grid.
    // Placement is determined by the PreferredIndex (0-14) of each button.
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid")
	TArray<TObjectPtr<URTSCommandButton>> Buttons;

	// Returns all buttons in this grid. Can be overridden by subclasses for code-defined grids.
	UFUNCTION(BlueprintCallable, Category = "Grid")
	virtual TArray<URTSCommandButton*> GetAllButtons() const
	{
		TArray<URTSCommandButton*> Result;
		for (auto& Btn : Buttons) if (Btn) Result.Add(Btn);
		return Result;
	}
};
