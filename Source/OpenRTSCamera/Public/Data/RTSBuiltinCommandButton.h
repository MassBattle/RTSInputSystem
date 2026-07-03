// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/RTSCommandButton.h"
#include "RTSBuiltinCommandButton.generated.h"

/**
 * Base class for "Code-Defined" Command Buttons.
 * Users can subclass this in C++ to define a button without creating an asset file.
 * The Default Object (CDO) of the subclass will be used as the button data.
 * 
 * Example:
 * UCLASS()
 * class URTSCmd_BuildFactory : public URTSBuiltinCommandButton
 * {
 *     virtual void PostInitProperties() override; // Set Defaults here
 * };
 */
UCLASS(Abstract, Blueprintable)
class OPENRTSCAMERA_API URTSBuiltinCommandButton : public URTSCommandButton
{
	GENERATED_BODY()
	
public:
    virtual void PostInitProperties() override;
};
