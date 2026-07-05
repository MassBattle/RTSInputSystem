// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSSelectionStructs.h"
#include "RTSFormationListWidget.generated.h"

class UPanelWidget;
class URTSUnitIconWidget;

/**
 * Independent formation/control-group strip inside the UnitPanel header.
 *
 * This widget is not the selection roster and does not decide UnitPanel size.
 * It only visualizes the current high-level groups exposed by FRTSSelectionView.
 */
UCLASS(BlueprintType, Blueprintable)
class OPENRTSCAMERA_API URTSFormationListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

protected:
	UFUNCTION()
	virtual void OnSelectionUpdated(const FRTSSelectionView& View);

	/** Icon widget used for each formation/group indicator. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Formation List")
	TSubclassOf<URTSUnitIconWidget> FormationIconClass;

	/** Maximum visible formation/group indicators. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Formation List", meta = (ClampMin = "1"))
	int32 MaxFormationSlots = 8;

	/** Square size for each formation/group indicator. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Formation List", meta = (ClampMin = "1"))
	int32 FormationIconSize = 32;

	/** Gap between formation/group indicators. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Formation List", meta = (ClampMin = "0"))
	float FormationSlotGap = 4.0f;

	/** Container authored by the UMG asset. Usually a HorizontalBox. */
	UPROPERTY(meta = (BindWidget))
	UPanelWidget* FormationSlotContainer;

	UFUNCTION(BlueprintImplementableEvent, Category = "RTS Formation List")
	void OnFormationListChanged(const FRTSSelectionView& View);

private:
	UPROPERTY()
	TArray<URTSUnitIconWidget*> FormationSlots;

	void ApplyFormationSettings();
	void BuildSlotPool();
	void RefreshFormationList(const FRTSSelectionView& View);
	void HideSlots();
};
