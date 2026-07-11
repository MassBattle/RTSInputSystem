// Copyright 2024 Jesus Bracho All Rights Reserved.

#include "RTSInputSystem.h"
#include "GameplayTagsManager.h"

#define LOCTEXT_NAMESPACE "FRTSInputSystemModule"

DEFINE_LOG_CATEGORY(LogRTSInputSystem);

void FRTSInputSystemModule::StartupModule()
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
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Navy.OpenProduction")), TEXT("Open naval production panel."));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Air.OpenProduction")), TEXT("Open air production panel."));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Decision.OpenPanel")), TEXT("Open player decision panel."));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Market.OpenForeignUnits")), TEXT("Open foreign arms market panel."));
	TagsManager.AddNativeGameplayTag(FName(TEXT("RTS.Command.Research.OpenPanel")), TEXT("Open research panel."));
}

void FRTSInputSystemModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FRTSInputSystemModule, RTSInputSystem)
