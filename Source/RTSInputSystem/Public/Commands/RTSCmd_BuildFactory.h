// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/RTSBuiltinCommandButton.h"
#include "RTSCmd_BuildFactory.generated.h"

/**
 * Built-in Command: Build Factory
 * - Cost: 100 Minerals (Mock)
 * - CD: 75s
 * - Auto-Cast: Yes
 */
UCLASS()
class RTSINPUTSYSTEM_API URTSCmd_BuildFactory : public URTSBuiltinCommandButton
{
	GENERATED_BODY()

public:
	URTSCmd_BuildFactory()
	{
		// Setup Defaults
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Build.Factory"), false);
		TargetType = ERTSCommandTargetType::Location;
		
        // Note: Loading assets in constructor can be risky for cooked builds if not careful, 
        // but standard for CDO. 
        // Ideally we use TSoftObjectPtr or just leave Icon null for now (User can assign in BP subclass if they want)
        // Or we use a placeholder if available.
        
		DisplayName = FText::FromString(TEXT("建造工厂"));
		
		FString DescStr = TEXT("建造工厂，提升3点GDP。<n/><n/><RichText.Yellow>定位： 经济。</><n/><n/><RichText.Red><RichText.Green><n/>四，五级城市GDP +1/+2<n/></><RichText.Green><n/>平原/高原GDP -1/-2<n/></>");
        // Formatting fixes for XML/RichText
		Description = FText::FromString(DescStr);

		PreferredIndex = 5; // Row 2, Col 1
		DefaultCooldown = 75.0f;
		bAllowAutoCast = true;
	}
};

