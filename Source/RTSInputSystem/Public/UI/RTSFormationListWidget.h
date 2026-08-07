// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSSelectionStructs.h"
#include "RTSFormationListWidget.generated.h"

class UPanelWidget;
class USizeBox;
class UUniformGridPanel;
class URTSControlGroupButton;

/** Persistent 0-9 control-group strip for the bottom UnitDetailPanel header. */
UCLASS(BlueprintType, Blueprintable)
class RTSINPUTSYSTEM_API URTSFormationListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	virtual void OnControlGroupsUpdated(const FRTSControlGroupsView& View);

	/** Optional visual subclass. Native URTSControlGroupButton is used when unset. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Control Groups")
	TSubclassOf<URTSControlGroupButton> ControlGroupButtonClass;

	/** The runtime supports keyboard groups 0-9; values are clamped to ten. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Control Groups", meta = (ClampMin = "1", ClampMax = "10"))
	int32 MaxFormationSlots = 10;

	/** Readable card geometry; defaults align one card with one selection-grid column. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Control Groups", meta = (ClampMin = "1"))
	int32 FormationSlotWidth = 128;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Control Groups", meta = (ClampMin = "1"))
	int32 FormationSlotHeight = 64;

	/** Visible assigned cards wrap after this many columns. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Control Groups", meta = (ClampMin = "1", ClampMax = "10"))
	int32 FormationColumns = 8;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Control Groups", meta = (ClampMin = "0"))
	float FormationSlotGap = 4.0f;

	/** Existing UnitFormationList asset already authors this exact root name. */
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* FormationSlotContainer;

	UFUNCTION(BlueprintImplementableEvent, Category = "RTS Control Groups")
	void OnControlGroupListChanged(const FRTSControlGroupsView& View);

private:
	UPROPERTY()
	TArray<URTSControlGroupButton*> ControlGroupButtons;

	UPROPERTY()
	TArray<USizeBox*> ControlGroupSlotBoxes;

	UPROPERTY(Transient)
	TObjectPtr<UUniformGridPanel> ControlGroupGrid;

	void ApplyFormationSettings();
	void BuildSlotPool();
	void RefreshControlGroups(const FRTSControlGroupsView& View);
};
