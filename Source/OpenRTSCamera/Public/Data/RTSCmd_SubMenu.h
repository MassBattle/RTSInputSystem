// Copyright 2026 Winyunq. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/RTSBuiltinCommandButton.h"
#include "RTSCmd_SubMenu.generated.h"

/**
 * A specialized button that opens another command grid (Sub-menu).
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_SubMenu : public URTSBuiltinCommandButton
{
	GENERATED_BODY()

public:
    /** The grid to open when this button is clicked. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Command")
    TObjectPtr<class URTSCommandGridAsset> TargetGrid;

    virtual void Execute_Implementation(AActor* Executor) override;
};
