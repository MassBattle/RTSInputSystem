// Copyright 2024 Winy unq All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/RTSCommandButton.h"
#include "RTSTooltipWidget.generated.h"

class UTextBlock;
class UImage;
class UTexture2D;
class UDataTable;

/**
 * A Rich Tooltip for RTS Commands.
 * Displays Name, Description, Cost, Cooldown.
 */
UCLASS()
class RTSINPUTSYSTEM_API URTSTooltipWidget : public UUserWidget
{
	GENERATED_BODY()

public:

	// Update Tooltip UI from Data
	UFUNCTION(BlueprintCallable, Category = "RTS Tooltip")
	void UpdateTooltip(URTSCommandButton* Data);

	UFUNCTION(BlueprintCallable, Category = "RTS Tooltip")
	void SetTooltipContent(const FText& InTitle, const FText& InDescription, const FText& InCost, UTexture2D* InIcon);

protected:
	virtual TSharedRef<SWidget> RebuildWidget() override;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> TitleText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<class URichTextBlock> DescriptionText;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> CostText; // "100 M / 50 G"

	// Optional icon
    UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> IconImage;

    // --- Style Config (Start) ---
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (ClampMin = "8", ClampMax = "64"))
    int32 DefaultFontSize = 32;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (ClampMin = "8", ClampMax = "64"))
    int32 DescriptionFontSize = 16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (ClampMin = "8", ClampMax = "64"))
    int32 CostFontSize = 16;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style", meta = (ClampMin = "120.0", ClampMax = "800.0"))
    float TooltipWrapTextAt = 512.0f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
    TObjectPtr<UDataTable> UnifiedRichTextStyleSet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
    FLinearColor TitleColor = FLinearColor(0.78f, 0.68f, 0.43f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
    FLinearColor DescriptionColor = FLinearColor(0.78f, 0.76f, 0.68f, 1.0f);

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Style")
    FLinearColor CostColor = FLinearColor(0.46f, 0.78f, 0.42f, 1.0f);
    
    virtual void NativeConstruct() override;

private:
	void ApplyConfiguredStyle();
};
