// Copyright 2024 Winy unq All Rights Reserved.

#include "Data/RTSBuiltinCommandButton.h"
#include "GameplayTagsManager.h"

void URTSBuiltinCommandButton::PostInitProperties()
{
	Super::PostInitProperties();

	// If tag is not manually set, generate it from Class Name
	// This allows C++ buttons to "just work" without manual registration in Project Settings.
	if (!CommandTag.IsValid())
	{
		FString ClassName = GetClass()->GetName();
		
		// Clean up common prefixes
		ClassName.RemoveFromStart(TEXT("URTSCmd_"));
		ClassName.RemoveFromStart(TEXT("RTSCmd_"));
		ClassName.RemoveFromStart(TEXT("URTS_"));
		ClassName.RemoveFromStart(TEXT("RTS_"));

		FString TagString = FString::Printf(TEXT("RTS.Command.Builtin.%s"), *ClassName);
		FName TagName(*TagString);

		// Register natively at runtime if needed
		UGameplayTagsManager::Get().AddNativeGameplayTag(TagName, FString::Printf(TEXT("Auto-generated tag for %s"), *ClassName));
		
		// Request the tag (it will now exist)
		CommandTag = FGameplayTag::RequestGameplayTag(TagName, false);
	}
}
