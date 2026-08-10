// Copyright 2024 Winy unq All Rights Reserved.

#include "UI/RTSActiveGroupWidget.h"
#include "UI/RTSUnitIconWidget.h"
#include "RTSSelectionSubsystem.h" 
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"

namespace
{
	FString GetActiveGroupUnitGroupKey(const FRTSUnitData& Data)
	{
		return Data.GroupKey.IsEmpty() ? Data.Name : Data.GroupKey;
	}
}

TSharedRef<SWidget> URTSActiveGroupWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		USizeBox* RootSize = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(), TEXT("DefaultActiveGroupSize"));
		RootSize->SetWidthOverride(160.0f);
		RootSize->SetHeightOverride(160.0f);

		UBorder* Background = WidgetTree->ConstructWidget<UBorder>(
			UBorder::StaticClass(), TEXT("DefaultActiveGroupBackground"));
		Background->SetBrushColor(FLinearColor(0.012f, 0.035f, 0.052f, 0.97f));
		Background->SetPadding(FMargin(8.0f));
		RootSize->AddChild(Background);

		GroupIcon = WidgetTree->ConstructWidget<URTSUnitIconWidget>(
			URTSUnitIconWidget::StaticClass(), TEXT("GroupIcon"));
		Background->SetContent(GroupIcon);
		WidgetTree->RootWidget = RootSize;
	}

	return Super::RebuildWidget();
}

void URTSActiveGroupWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (URTSSelectionSubsystem* Subsystem = LP->GetSubsystem<URTSSelectionSubsystem>())
			{
				Subsystem->OnSelectionChanged.AddUniqueDynamic(this, &URTSActiveGroupWidget::OnSelectionUpdated);
			}
		}
	}
}

void URTSActiveGroupWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (URTSSelectionSubsystem* Subsystem = LP->GetSubsystem<URTSSelectionSubsystem>())
			{
				Subsystem->OnSelectionChanged.RemoveDynamic(this, &URTSActiveGroupWidget::OnSelectionUpdated);
			}
		}
	}

	Super::NativeDestruct();
}

void URTSActiveGroupWidget::OnSelectionUpdated(const FRTSSelectionView& View)
{
	const FRTSUnitData* ActiveData = nullptr;
	FString ActiveKey = View.ActiveGroupKey;
	
	if (!ActiveKey.IsEmpty())
	{
		ActiveData = View.Items.FindByPredicate([&](const FRTSUnitData& Item) {
			return GetActiveGroupUnitGroupKey(Item) == ActiveKey;
		});
	}

	// Fallback: If no ActiveKey but items exist, use first item.
	if (!ActiveData && View.Items.Num() > 0)
	{
		ActiveData = &View.Items[0];
	}

	if (ActiveData)
	{
		// We have an active group/unit.
		// If we wrap an internal icon widget, update it.
		if (GroupIcon)
		{
			// Active avatar uses the pushed portrait when available; roster cells keep their small icon.
			FRTSUnitData AvatarData = *ActiveData;
			if (AvatarData.Portrait)
			{
				AvatarData.Icon = AvatarData.Portrait;
			}
			GroupIcon->InitData(AvatarData, true, true);
			GroupIcon->SetIsActive(true);
		}

		if (AvatarImage)
		{
			UTexture2D* AvatarTexture = ActiveData->Portrait
				? ActiveData->Portrait
				: ActiveData->Icon;
			if (AvatarTexture)
			{
				AvatarImage->SetBrushFromTexture(AvatarTexture);
				AvatarImage->SetColorAndOpacity(FLinearColor::White);
				AvatarImage->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else
			{
				AvatarImage->SetVisibility(ESlateVisibility::Hidden);
			}
		}
		
		// Ensure self is visible (hit test invisible to allow tooltips on children)
		SetVisibility(ESlateVisibility::SelfHitTestInvisible);
		
		// Notify BP
		OnActiveGroupChanged(*ActiveData, true);
	}
	else
	{
		// No selection at all.
		SetVisibility(ESlateVisibility::Hidden);
		if (AvatarImage)
		{
			AvatarImage->SetVisibility(ESlateVisibility::Hidden);
		}
		
		// Notify BP (Empty Data)
		OnActiveGroupChanged(FRTSUnitData(), false);
	}
}
