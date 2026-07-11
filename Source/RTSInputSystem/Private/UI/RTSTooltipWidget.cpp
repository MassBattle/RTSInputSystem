// Copyright 2024 Winy unq All Rights Reserved.

#include "UI/RTSTooltipWidget.h"
#include "Components/TextBlock.h"
#include "Components/RichTextBlock.h"
#include "Components/Image.h"
#include "Engine/DataTable.h"

namespace
{
	const TCHAR* UnifiedTooltipRichTextStyleSetPath = TEXT("/Game/UI/HeadUpDisplay/RTSStyle/DT_RTS_UnifiedRichTextStyle.DT_RTS_UnifiedRichTextStyle");
}

void URTSTooltipWidget::NativeConstruct()
{
    Super::NativeConstruct();

    ApplyConfiguredStyle();
}

void URTSTooltipWidget::ApplyConfiguredStyle()
{
    if (TitleText)
    {
        FSlateFontInfo Font = TitleText->GetFont();
        Font.Size = DefaultFontSize;
        TitleText->SetFont(Font);
        TitleText->SetColorAndOpacity(FSlateColor(TitleColor));
        TitleText->SetAutoWrapText(true);
        TitleText->SetWrapTextAt(TooltipWrapTextAt);
    }

    if (DescriptionText)
    {
        if (!UnifiedRichTextStyleSet)
        {
            UnifiedRichTextStyleSet = LoadObject<UDataTable>(nullptr, UnifiedTooltipRichTextStyleSetPath);
        }

        if (UnifiedRichTextStyleSet)
        {
            DescriptionText->SetTextStyleSet(UnifiedRichTextStyleSet);
        }

        FTextBlockStyle DefaultStyle = DescriptionText->GetDefaultTextStyle();
        DefaultStyle.Font.Size = DescriptionFontSize;
        DefaultStyle.SetColorAndOpacity(FSlateColor(DescriptionColor));
        DescriptionText->SetDefaultTextStyle(DefaultStyle);
        DescriptionText->SetAutoWrapText(true);
        DescriptionText->SetWrapTextAt(TooltipWrapTextAt);
    }

    if (CostText)
    {
        FSlateFontInfo Font = CostText->GetFont();
        Font.Size = CostFontSize;
        CostText->SetFont(Font);
        CostText->SetColorAndOpacity(FSlateColor(CostColor));
    }
}

void URTSTooltipWidget::UpdateTooltip(URTSCommandButton* Data)
{
	if (!Data) return;

    FString CostStr;
    if (Data->LowValueCost > 0)
    {
        CostStr += FString::Printf(TEXT("%d 资金"), Data->LowValueCost);
    }
    if (Data->HighValueCost > 0)
    {
        if (!CostStr.IsEmpty())
        {
            CostStr += TEXT(" / ");
        }
        CostStr += FString::Printf(TEXT("%d 军需"), Data->HighValueCost);
    }

    SetTooltipContent(Data->DisplayName, Data->Description, FText::FromString(CostStr), Data->Icon);
}

void URTSTooltipWidget::SetTooltipContent(const FText& InTitle, const FText& InDescription, const FText& InCost, UTexture2D* InIcon)
{
    if (TitleText)
    {
        TitleText->SetText(InTitle);
        TitleText->SetVisibility(InTitle.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }

    if (DescriptionText)
    {
        DescriptionText->SetText(InDescription);
        DescriptionText->SetVisibility(InDescription.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }

    if (CostText)
    {
        CostText->SetText(InCost);
        CostText->SetVisibility(InCost.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);
    }

    if (IconImage)
    {
        if (InIcon)
        {
            IconImage->SetBrushFromTexture(InIcon);
            IconImage->SetVisibility(ESlateVisibility::HitTestInvisible);
        }
        else
        {
            IconImage->SetVisibility(ESlateVisibility::Collapsed);
        }
    }

    ApplyConfiguredStyle();
}
