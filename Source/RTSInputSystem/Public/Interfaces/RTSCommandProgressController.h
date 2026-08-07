// Copyright 2026 Winyunq. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "RTSCommandProgressController.generated.h"

/**
 * Optional action endpoint for a common command-progress item.
 *
 * The selection UI knows only the stable item id. Gameplay systems remain
 * responsible for authority, refunds, queue promotion, and state refresh.
 */
UINTERFACE(BlueprintType)
class RTSINPUTSYSTEM_API URTSCommandProgressController : public UInterface
{
	GENERATED_BODY()
};

class RTSINPUTSYSTEM_API IRTSCommandProgressController
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "RTS Progress")
	void RequestCancelCommandProgressItem(FName ItemId);
};
