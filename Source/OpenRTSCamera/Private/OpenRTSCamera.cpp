// Copyright 2024 Jesus Bracho All Rights Reserved.

#include "OpenRTSCamera.h"
#include "GameplayTagsManager.h"

#define LOCTEXT_NAMESPACE "FOpenRTSCameraModule"

DEFINE_LOG_CATEGORY(LogOpenRTSCamera);

void FOpenRTSCameraModule::StartupModule()
{
	UGameplayTagsManager& TagsManager = UGameplayTagsManager::Get();
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Move")), TEXT("Default RTS unit move command"));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Attack")), TEXT("Default RTS unit attack command"));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Stop")), TEXT("Default RTS unit stop command"));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Hold")), TEXT("Default RTS unit hold command"));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Patrol")), TEXT("Default RTS unit patrol command"));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Build.Factory")), TEXT("Default RTS build command"));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Build.University")), TEXT("Default RTS build command"));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Build.Barracks")), TEXT("Default RTS build command"));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Build.VehicleDepot")), TEXT("Default RTS build command"));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Build.Airfield")), TEXT("Default RTS build command"));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Train.Officer")), TEXT("Default RTS train command"));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Train.Militia")), TEXT("Default RTS train command"));
}

void FOpenRTSCameraModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOpenRTSCameraModule, OpenRTSCamera)
