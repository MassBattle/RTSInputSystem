// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/RTSBuiltinCommandButton.h"
#include "Data/RTSCommandGridAsset.h"
#include "UObject/UObjectGlobals.h"
#include "RTSUnitCommands.generated.h"

// --- Row 1: Basic SC-Style Commands ---

/**
 * Move (M)
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_Move : public URTSBuiltinCommandButton
{
	GENERATED_BODY()
public:
	URTSCmd_Move()
	{
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Move"), false);
		TargetType = ERTSCommandTargetType::Location; // Require click on ground
		DisplayName = FText::FromString(TEXT("移动"));
		Description = FText::FromString(TEXT("移动到指定位置。<n/><n/><RichText.Yellow>快捷键: Q 或 右键点击</>"));
		PreferredIndex = 0; // Row 1, Col 1
		Hotkey = EKeys::Q;
	}
};

/**
 * Attack (A)
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_Attack : public URTSBuiltinCommandButton
{
	GENERATED_BODY()
public:
	URTSCmd_Attack()
	{
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Attack"), false);
		TargetType = ERTSCommandTargetType::LocationOrTarget; // Can click ground or enemy
		DisplayName = FText::FromString(TEXT("攻击"));
		Description = FText::FromString(TEXT("向目标位置移动并攻击沿途敌人。<n/><n/><RichText.Yellow>快捷键: W</>"));
		PreferredIndex = 1; // Row 1, Col 2
		Hotkey = EKeys::W;
	}
};

/**
 * Stop (S)
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_Stop : public URTSBuiltinCommandButton
{
	GENERATED_BODY()
public:
	URTSCmd_Stop()
	{
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Stop"), false);
		TargetType = ERTSCommandTargetType::Instant;
		DisplayName = FText::FromString(TEXT("停止"));
		Description = FText::FromString(TEXT("立即停止当前所有行动。<n/><n/><RichText.Yellow>快捷键: E</>"));
		PreferredIndex = 2; // Row 1, Col 3
		Hotkey = EKeys::E;
	}
};

/**
 * Hold Position (H)
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_HoldPosition : public URTSBuiltinCommandButton
{
	GENERATED_BODY()
public:
	URTSCmd_HoldPosition()
	{
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Hold"), false);
		TargetType = ERTSCommandTargetType::Instant;
		DisplayName = FText::FromString(TEXT("驻守"));
		Description = FText::FromString(TEXT("坚守阵地，不追击敌人。<n/><n/><RichText.Yellow>快捷键: R</>"));
		PreferredIndex = 3; // Row 1, Col 4
		Hotkey = EKeys::R;
	}
};

/**
 * Patrol (P)
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_Patrol : public URTSBuiltinCommandButton
{
	GENERATED_BODY()
public:
	URTSCmd_Patrol()
	{
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Patrol"), false);
		TargetType = ERTSCommandTargetType::Location;
		DisplayName = FText::FromString(TEXT("巡逻"));
		Description = FText::FromString(TEXT("在当前位置和目标位置之间巡逻。<n/><n/><RichText.Yellow>快捷键: T</>"));
		PreferredIndex = 4; // Row 1, Col 5
		Hotkey = EKeys::T;
	}
};

/**
 * (New) Basic Unit Command Grid
 * Handles the logic-defined 5 buttons for Unit movement/combat.
 */
UCLASS()
class OPENRTSCAMERA_API URTSUnitCommandGrid : public URTSCommandGridAsset
{
    GENERATED_BODY()

public:
    virtual TArray<URTSCommandButton*> GetAllButtons() const override
    {
        TArray<URTSCommandButton*> Result;
        
        Result.Add(NewObject<URTSCmd_Move>());
        Result.Add(NewObject<URTSCmd_Attack>());
        Result.Add(NewObject<URTSCmd_Stop>());
        Result.Add(NewObject<URTSCmd_HoldPosition>());
        Result.Add(NewObject<URTSCmd_Patrol>());

        return Result;
    }
};

