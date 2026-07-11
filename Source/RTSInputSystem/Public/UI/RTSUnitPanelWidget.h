// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSSelectionStructs.h"
#include "RTSUnitPanelWidget.generated.h"

class UPanelWidget;
class UBorder;
class UTextBlock;
class UWidget;
class URTSUnitIconWidget;
class UProgressBar;
class SBox;
class SWidget;

/**
 * Main Selection Panel. Handles Empty, Single, List, and Summary routes.
 */
UCLASS()
class RTSINPUTSYSTEM_API URTSUnitPanelWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;
	virtual void ReleaseSlateResources(bool bReleaseChildren) override;

	UFUNCTION()
	void OnSelectionUpdated(const FRTSSelectionView& View);

	/**
	* Class of the item widget to spawn in the list.
	* Must be set in Blueprint (WBP_RTSUnitIcon).
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
	TSubclassOf<URTSUnitIconWidget> UnitIconClass;

	// The class to use for each unit icon. 
	// If set in Editor, we use this. If nullptr, we try to detect from the first child in Designer.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
	TSubclassOf<UUserWidget> IconWidgetClass;

	// Optional: The class for the "Count" widget in Summary mode.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
	TSubclassOf<UUserWidget> CountWidgetClass;

	// -- Bind Widgets --
	
	/**
	 * Max items to show in the grid. 
	 * Calculated automatically as MaxRows * MaxColumns.
	 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "RTS Selection")
	int32 ItemsPerPage = 24;

	/** Fixed square size for each selection panel cell. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
	int32 IconSlotSize = 128;

	/** Fixed header reserve for formation/control-group information inside UnitPanel. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
	float PanelHeaderHeight = 44.0f;

	/**
	* Max columns for the grid. Defaults to 8.
	* Can be overridden by explicit GridPanel column fill settings; template children do not define capacity.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
	int32 MaxColumns = 8;

	/**
	* Max rows for the grid. Defaults to 3.
	* Can be overridden by explicit GridPanel row fill settings; template children do not define capacity.
	*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Selection")
	int32 MaxRows = 3;

	// Optional UnitPanel visual shell. The C++ widget owns fixed outer bounds.
	UPROPERTY(meta = (BindWidgetOptional))
	UBorder* UnitPanelFrame;

	// Optional detail stack shown beside the roster.
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* UnitDetailPane;

	// List/Summary route content. Hidden for Single route.
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* UnitRosterPane;

	// Spacer between mutually exclusive routes when they are authored as siblings.
	UPROPERTY(meta = (BindWidgetOptional))
	UWidget* UnitPanelBodyGap;

	// UnitPanel list/summary content grid. This is content, not the UnitPanel shell.
	// Child 0 can be a unit icon template; runtime builds the fixed grid pool from it.
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* IconContainer;

private:
	void ApplySelectionPanelLayoutSettings();
	void ApplyFixedPanelSlotLayout();
	FVector2D CalculateFixedPanelSize() const;
	void ApplyFixedPanelBounds();

	// Pool of re-usable icon widgets
	UPROPERTY()
	TArray<URTSUnitIconWidget*> IconSlots;

	// Pool of re-usable count widgets (for Summary mode)
	UPROPERTY()
	TArray<UTextBlock*> CountSlots;

	TSharedPtr<SBox> FixedPanelBoundsBox;

	void RefreshGrid(const FRTSSelectionView& View);
	void ShowEmptyContent();
	void ShowSingleContent(const FRTSUnitData& Data);
	void ShowGridContent(const FRTSSelectionView& View);
	void RefreshSingleUnitDetail(const FRTSUnitData& Data);
	void HideGridSlots();
	UWidget* FindDescendantWidgetByName(UWidget* RootWidget, FName WidgetName) const;
};
