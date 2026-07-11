// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Data/RTSCommandButton.h"
#include "Data/RTSCommandGridAsset.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "UObject/SoftObjectPtr.h"
#include "RTSInputPanelSettings.generated.h"

USTRUCT(BlueprintType)
struct FRTSUnitAvatarDefinition
{
	GENERATED_BODY()

	/** Name shown in RTS selection UI. Array index maps to MassBattle FSubType.Index. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Avatar")
	FString DisplayName;

	/** Optional portrait for the RTS avatar panel. Mass entities do not store this. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Avatar")
	TSoftObjectPtr<UTexture2D> Avatar;
};

USTRUCT(BlueprintType)
struct FRTSCommandPanelSlot
{
	GENERATED_BODY()

	/** Keyboard shortcut for this command panel slot. Slots are 0-14, left-to-right, top-to-bottom. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Command Panel")
	FName Hotkey;
};

USTRUCT(BlueprintType)
struct FRTSMassUnitCommandSlotDefinition
{
	GENERATED_BODY()

	/** Slot index in the 15-button command card. Missing indices remain empty/hidden. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol", meta = (ClampMin = "0", ClampMax = "14"))
	int32 SlotIndex = -1;

	/** Logical command to issue when this button is clicked. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	FGameplayTag CommandTag;

	/** Config-friendly command tag name. Used when CommandTag is not set. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	FName CommandTagName;

	/** Button title shown in tooltips and optional labels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	FString DisplayName;

	/** Tooltip body. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	FString Description;

	/** Command targeting model. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	ERTSCommandTargetType TargetType = ERTSCommandTargetType::Instant;

	/** Optional slot hotkey override. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	FName Hotkey;

	/** Optional per-command icon. If unset, the CommandButton widget keeps its default artwork. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	TSoftObjectPtr<UTexture2D> Icon;

	/** If true, command availability checks may hide this button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	bool bHideIfUnavailable = false;
};

USTRUCT(BlueprintType)
struct FRTSMassUnitTypeProtocol
{
	GENERATED_BODY()

	/** MassBattle FSubType.Index. If left at -1, the array index is used as a fallback. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	int32 SubTypeIndex = INDEX_NONE;

	/** Stable type key used by UI grouping and future protocols. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	FString TypeKey;

	/** Optional gameplay tag for higher-level UI/audio routing. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	FGameplayTag UnitTypeTag;

	/** Name shown in the selection panel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	FString DisplayName;

	/** Short role/class text for detail panels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	FString Role;

	/** Small icon for selection list/summary. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	TSoftObjectPtr<UTexture2D> Icon;

	/** Larger avatar/portrait for the left-side single-selection detail panel. Separate from Icon. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	TSoftObjectPtr<UTexture2D> Portrait;

	/** Announcer cue key. Audio systems can resolve this now or later. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	FName AnnouncerId;

	/** Optional selected voice. Kept soft so selection does not force-load all unit audio. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	TSoftObjectPtr<USoundBase> SelectionSound;

	/** Optional command acknowledgement voice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	TSoftObjectPtr<USoundBase> ConfirmationSound;

	/** Optional authored command grid asset. Takes priority over CommandSlots. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	TSoftObjectPtr<URTSCommandGridAsset> CommandGrid;

	/** If no CommandGrid/CommandSlots are configured, fall back to the built-in Move/Attack/Stop/Hold/Patrol grid. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	bool bUseDefaultCommandGrid = true;

	/** Sparse command card entries. Only configured slots appear. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Mass Unit Protocol")
	TArray<FRTSMassUnitCommandSlotDefinition> CommandSlots;
};

UCLASS(Config=RTSInputSystem, DefaultConfig, BlueprintType)
class RTSINPUTSYSTEM_API URTSInputPanelSettings : public UObject
{
	GENERATED_BODY()

public:
	/** RTS-only MassBattle avatar mapping. Array index maps directly to FSubType.Index. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Avatar")
	TArray<FRTSUnitAvatarDefinition> MassUnitAvatars;

	/** Comprehensive MassBattle FSubType.Index -> RTS UI/command/audio protocol mapping. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Mass Unit Protocol")
	TArray<FRTSMassUnitTypeProtocol> MassUnitTypeProtocols;

	/** RTS command panel keyboard layout. Default is QWERT / ASDFG / ZXCVB. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Command Panel")
	TArray<FRTSCommandPanelSlot> CommandPanelSlots;

	/** Enables keyboard execution of visible command panel buttons. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Command Panel")
	bool bEnableCommandPanelHotkeys = true;

	/** Minimum font size applied to mission/objective rich text rows loaded from the shared style table. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Mission Panel", meta = (ClampMin = "1"))
	int32 MissionRichTextMinimumFontSize = 48;

	/** Selection counts above this value are exposed as grouped unit-type summaries instead of individual list items. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Selection Panel", meta = (ClampMin = "1"))
	int32 SelectionSummaryThreshold = 16;

	/** Number of rows generated for the RTS unit selection grid page. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Selection Panel", meta = (ClampMin = "1"))
	int32 SelectionGridRows = 3;

	/** Number of columns generated for the RTS unit selection grid page. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Selection Panel", meta = (ClampMin = "1"))
	int32 SelectionGridColumns = 8;

	/** Square pixel size used by each RTS unit portrait in the selection panel. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Selection Panel", meta = (ClampMin = "1"))
	int32 SelectionIconSize = 128;

	/** Fixed height reserved for UnitPanel header content such as formation information. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Selection Panel", meta = (ClampMin = "0"))
	float SelectionPanelHeaderHeight = 44.0f;

	/** Padding inside the fixed UnitPanel shell around the generated selection grid. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Selection Panel")
	FMargin SelectionPanelContentPadding = FMargin(16.0f, 4.0f, 16.0f, 4.0f);

	/** Maximum visible formation/group indicators shown in the UnitPanel header strip. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Formation List", meta = (ClampMin = "1"))
	int32 FormationListMaxSlots = 8;

	/** Square pixel size used by each formation/group indicator. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Formation List", meta = (ClampMin = "1"))
	int32 FormationListIconSize = 32;

	/** Horizontal gap between formation/group indicators. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Formation List", meta = (ClampMin = "0"))
	float FormationListSlotGap = 4.0f;

	/** Shows a world-space decal preview while hash-grid selection is active. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Hash Grid Selection")
	bool bEnableHashGridSelectionPreview = true;

	/** Deferred decal material used for hash-grid selection footprint preview. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Hash Grid Selection")
	TSoftObjectPtr<UMaterialInterface> HashGridSelectionDecalMaterial;

	/** World-space size of one hash-grid cell. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Hash Grid Selection", meta = (ClampMin = "1.0"))
	float HashGridCellSize = 400.0f;

	/** Footprint dimensions, in hash-grid cells, displayed by the decal. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Hash Grid Selection")
	FVector2D HashGridSelectionFootprintCells = FVector2D(3.0f, 3.0f);

	/** Decal projection depth. Increase this if steep terrain stops receiving the preview. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Hash Grid Selection", meta = (ClampMin = "1.0"))
	float HashGridSelectionDecalDepth = 4096.0f;

	/** Half-height used when tracing snapped hash-grid locations back down to terrain. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Hash Grid Selection", meta = (ClampMin = "1000.0"))
	float HashGridSelectionTraceHalfHeight = 50000.0f;

	/** Snaps selected X/Y to HashGridCellSize before issuing the command. */
	UPROPERTY(Config, EditAnywhere, BlueprintReadOnly, Category = "RTS Hash Grid Selection")
	bool bSnapHashGridSelectionToGrid = true;
};
