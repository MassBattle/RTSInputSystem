// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/RTSCommandGridAsset.h"
#include "Data/RTSBuiltinCommandButton.h"
#include "RTSCmd_BuildFactory.h"
#include "RTSCityCommands.generated.h"

// --- ROW 2: Buildings ---

// Col 1: Factory (Already in separate file, but we can consolidate here or keep separate. User has separate file.)
// We will focus on the others.

/**
 * Col 2: University (Tech)
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_BuildUniversity : public URTSBuiltinCommandButton
{
	GENERATED_BODY()
public:
	URTSCmd_BuildUniversity()
	{
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Build.University"), false);
		DisplayName = FText::FromString(TEXT("建造西南联大")); // Flavor: Southwest Associated University
		Description = FText::FromString(TEXT("发展科技，提升科研效率。<n/><n/><RichText.Yellow>定位： 科技。</>"));
		PreferredIndex = 6; // Row 2, Col 2 (5+1)
		DefaultCooldown = 60.0f;
        bAllowAutoCast = false;
        
        LowValueCost = 150; // Minerals
        HighValueCost = 50; // Gas
	}
};

/**
 * Col 3: Barracks (Infantry)
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_BuildBarracks : public URTSBuiltinCommandButton
{
	GENERATED_BODY()
public:
	URTSCmd_BuildBarracks()
	{
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Build.Barracks"), false);
		DisplayName = FText::FromString(TEXT("建造训练营"));
		Description = FText::FromString(TEXT("招募步兵与军官。<n/><n/><RichText.Yellow>定位： 军事。</>"));
		PreferredIndex = 7; // Row 2, Col 3
		DefaultCooldown = 45.0f;
        
        LowValueCost = 150;
	}
};

/**
 * Col 4: Tank Factory (Mechanized) -> "Vehicles"
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_BuildVehicleDepot : public URTSBuiltinCommandButton
{
	GENERATED_BODY()
public:
	URTSCmd_BuildVehicleDepot()
	{
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Build.VehicleDepot"), false);
		DisplayName = FText::FromString(TEXT("建造辎重营")); // Flavor: Supply/Vehicle Depot
		Description = FText::FromString(TEXT("生产卡车与轻型装甲车。<n/><n/><RichText.Yellow>定位： 机械化。</>"));
		PreferredIndex = 8; // Row 2, Col 4
		DefaultCooldown = 90.0f;
        
        LowValueCost = 200;
        HighValueCost = 100;
	}
};

/**
 * Col 5: Airfield
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_BuildAirfield : public URTSBuiltinCommandButton
{
	GENERATED_BODY()
public:
	URTSCmd_BuildAirfield()
	{
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Build.Airfield"), false);
		DisplayName = FText::FromString(TEXT("建造简易机场"));
		Description = FText::FromString(TEXT("呼叫空军支援。<n/><n/><RichText.Yellow>定位： 空军。</>"));
		PreferredIndex = 9; // Row 2, Col 5
		DefaultCooldown = 120.0f;
        
        LowValueCost = 150;
        HighValueCost = 100;
	}
};

// --- ROW 3: Units ---

/**
 * Col 1: Officer (SCV equivalent)
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_TrainOfficer : public URTSBuiltinCommandButton
{
	GENERATED_BODY()
public:
	URTSCmd_TrainOfficer()
	{
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Train.Officer"), false);
		DisplayName = FText::FromString(TEXT("招募军官"));
		Description = FText::FromString(TEXT("负责前线建设与指挥。<n/><n/><RichText.Yellow>定位： 工程/指挥。</>"));
		PreferredIndex = 10; // Row 3, Col 1
		DefaultCooldown = 20.0f;
        bAllowAutoCast = true; // Auto-train?
        
        LowValueCost = 50;
	}
};

/**
 * Col 2: Militia
 */
UCLASS()
class OPENRTSCAMERA_API URTSCmd_TrainMilitia : public URTSBuiltinCommandButton
{
	GENERATED_BODY()
public:
	URTSCmd_TrainMilitia()
	{
		CommandTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Train.Militia"), false);
		DisplayName = FText::FromString(TEXT("动员民兵"));
		Description = FText::FromString(TEXT("基础防御单位。<n/><n/><RichText.Yellow>定位： 轻步兵。</>"));
		PreferredIndex = 11; // Row 3, Col 2
		DefaultCooldown = 15.0f;
        bAllowAutoCast = true;
        
        LowValueCost = 25;
	}
};

/**
 * (New) City Command Grid - Concrete implementation for all Cities
 * Handles the logic-defined 7 buttons for City management.
 */
UCLASS()
class OPENRTSCAMERA_API URTSCityCommandGrid : public URTSCommandGridAsset
{
    GENERATED_BODY()

public:
    virtual TArray<URTSCommandButton*> GetAllButtons() const override
    {
        TArray<URTSCommandButton*> Result;
        
        Result.Add(NewObject<URTSCmd_BuildFactory>());
        Result.Add(NewObject<URTSCmd_BuildUniversity>());
        Result.Add(NewObject<URTSCmd_BuildBarracks>());
        Result.Add(NewObject<URTSCmd_BuildVehicleDepot>());
        Result.Add(NewObject<URTSCmd_BuildAirfield>());
        Result.Add(NewObject<URTSCmd_TrainOfficer>());
        Result.Add(NewObject<URTSCmd_TrainMilitia>());

        return Result;
    }
};

