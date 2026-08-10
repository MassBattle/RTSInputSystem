#include "UI/RTSFormationListWidget.h"

#include "RTSInputPanelSettings.h"
#include "RTSSelectionSubsystem.h"
#include "UI/RTSControlGroupButton.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Image.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"
#include "Components/UniformGridPanel.h"
#include "Components/UniformGridSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

namespace
{
	const int32 FormationControlGroupDisplayOrder[] = { 1, 2, 3, 4, 5, 6, 7, 8, 9, 0 };
}

TSharedRef<SWidget> URTSFormationListWidget::RebuildWidget()
{
	if (WidgetTree && !WidgetTree->RootWidget)
	{
		UHorizontalBox* Root = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(), TEXT("FormationSlotContainer"));
		FormationSlotContainer = Root;
		WidgetTree->RootWidget = Root;
	}

	return Super::RebuildWidget();
}

void URTSFormationListWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyFormationSettings();
}

void URTSFormationListWidget::NativeConstruct()
{
	Super::NativeConstruct();
	ApplyFormationSettings();
	BuildSlotPool();

	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (URTSSelectionSubsystem* Selection = LP->GetSubsystem<URTSSelectionSubsystem>())
			{
				Selection->OnControlGroupsChanged.AddUniqueDynamic(this, &URTSFormationListWidget::OnControlGroupsUpdated);
				OnControlGroupsUpdated(Selection->GetControlGroupsView());
			}
		}
	}
}

void URTSFormationListWidget::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		if (ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			if (URTSSelectionSubsystem* Selection = LP->GetSubsystem<URTSSelectionSubsystem>())
			{
				Selection->OnControlGroupsChanged.RemoveDynamic(this, &URTSFormationListWidget::OnControlGroupsUpdated);
			}
		}
	}
	Super::NativeDestruct();
}

void URTSFormationListWidget::ApplyFormationSettings()
{
	if (const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>())
	{
		MaxFormationSlots = FMath::Clamp(Settings->FormationListMaxSlots, 1, 10);
		FormationSlotWidth = FMath::Max(1, Settings->FormationListSlotWidth);
		FormationSlotHeight = FMath::Max(1, Settings->FormationListSlotHeight);
		FormationColumns = FMath::Clamp(Settings->SelectionGridColumns, 1, 10);
		FormationSlotGap = FMath::Max(0.0f, Settings->FormationListSlotGap);
	}
}

void URTSFormationListWidget::BuildSlotPool()
{
	if (!FormationSlotContainer || !WidgetTree)
	{
		UE_LOG(LogTemp, Warning, TEXT("RTSFormationListWidget: FormationSlotContainer is not bound."));
		return;
	}

	FormationSlotContainer->ClearChildren();
	ControlGroupButtons.Reset();
	ControlGroupSlotBoxes.Reset();
	ControlGroupGrid = WidgetTree->ConstructWidget<UUniformGridPanel>(
		UUniformGridPanel::StaticClass(),
		TEXT("ControlGroupGrid"));
	if (!ControlGroupGrid)
	{
		return;
	}

	ControlGroupGrid->SetMinDesiredSlotWidth(FormationSlotWidth);
	ControlGroupGrid->SetMinDesiredSlotHeight(FormationSlotHeight);
	ControlGroupGrid->SetSlotPadding(FMargin(0.0f, 0.0f, 0.0f, FormationSlotGap));
	FormationSlotContainer->AddChild(ControlGroupGrid);
	if (UHorizontalBoxSlot* GridHostSlot = Cast<UHorizontalBoxSlot>(ControlGroupGrid->Slot))
	{
		GridHostSlot->SetHorizontalAlignment(HAlign_Left);
		GridHostSlot->SetVerticalAlignment(VAlign_Center);
	}

	if (!ControlGroupButtonClass)
	{
		ControlGroupButtonClass = URTSControlGroupButton::StaticClass();
	}

	const int32 SlotCount = FMath::Min(MaxFormationSlots, static_cast<int32>(UE_ARRAY_COUNT(FormationControlGroupDisplayOrder)));
	for (int32 SlotIndex = 0; SlotIndex < SlotCount; ++SlotIndex)
	{
		const int32 GroupIndex = FormationControlGroupDisplayOrder[SlotIndex];
		URTSControlGroupButton* Button = WidgetTree->ConstructWidget<URTSControlGroupButton>(
			ControlGroupButtonClass,
			FName(*FString::Printf(TEXT("ControlGroup_%d"), GroupIndex)));
		if (!Button)
		{
			continue;
		}
		Button->ControlGroupIndex = GroupIndex;

		UHorizontalBox* CardRow = WidgetTree->ConstructWidget<UHorizontalBox>(
			UHorizontalBox::StaticClass(),
			FName(*FString::Printf(TEXT("ControlGroupRow_%d"), GroupIndex)));
		USizeBox* PortraitBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("ControlGroupPortraitBox_%d"), GroupIndex)));
		UImage* Icon = WidgetTree->ConstructWidget<UImage>(
			UImage::StaticClass(),
			FName(*FString::Printf(TEXT("ControlGroupIcon_%d"), GroupIndex)));
		UVerticalBox* InfoBox = WidgetTree->ConstructWidget<UVerticalBox>(
			UVerticalBox::StaticClass(),
			FName(*FString::Printf(TEXT("ControlGroupInfo_%d"), GroupIndex)));
		UTextBlock* NumberText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("ControlGroupNumber_%d"), GroupIndex)));
		UTextBlock* CountText = WidgetTree->ConstructWidget<UTextBlock>(
			UTextBlock::StaticClass(),
			FName(*FString::Printf(TEXT("ControlGroupCount_%d"), GroupIndex)));

		if (!CardRow || !PortraitBox || !Icon || !InfoBox || !NumberText || !CountText)
		{
			continue;
		}

		Button->AddChild(CardRow);
		PortraitBox->SetWidthOverride(FormationSlotHeight);
		PortraitBox->SetHeightOverride(FormationSlotHeight);
		PortraitBox->SetContent(Icon);
		if (UHorizontalBoxSlot* PortraitSlot = CardRow->AddChildToHorizontalBox(PortraitBox))
		{
			FSlateChildSize AutoSize;
			AutoSize.SizeRule = ESlateSizeRule::Automatic;
			PortraitSlot->SetSize(AutoSize);
			PortraitSlot->SetHorizontalAlignment(HAlign_Fill);
			PortraitSlot->SetVerticalAlignment(VAlign_Fill);
		}

		if (UHorizontalBoxSlot* InfoSlot = CardRow->AddChildToHorizontalBox(InfoBox))
		{
			FSlateChildSize FillSize;
			FillSize.SizeRule = ESlateSizeRule::Fill;
			FillSize.Value = 1.0f;
			InfoSlot->SetSize(FillSize);
			InfoSlot->SetHorizontalAlignment(HAlign_Fill);
			InfoSlot->SetVerticalAlignment(VAlign_Fill);
			InfoSlot->SetPadding(FMargin(6.0f, 2.0f, 4.0f, 2.0f));
		}

		NumberText->SetText(FText::Format(FText::FromString(TEXT("编队 {0}")), FText::AsNumber(GroupIndex)));
		NumberText->SetColorAndOpacity(FSlateColor(FLinearColor(0.78f, 1.0f, 0.91f, 1.0f)));
		NumberText->SetShadowColorAndOpacity(FLinearColor::Black);
		NumberText->SetShadowOffset(FVector2D(1.0f, 1.0f));
		FSlateFontInfo NumberFont = NumberText->GetFont();
		NumberFont.Size = FMath::Max(14, FormationSlotHeight / 4);
		NumberText->SetFont(NumberFont);
		if (UVerticalBoxSlot* NumberSlot = InfoBox->AddChildToVerticalBox(NumberText))
		{
			FSlateChildSize AutoSize;
			AutoSize.SizeRule = ESlateSizeRule::Automatic;
			NumberSlot->SetSize(AutoSize);
			NumberSlot->SetHorizontalAlignment(HAlign_Left);
			NumberSlot->SetVerticalAlignment(VAlign_Center);
		}

		CountText->SetJustification(ETextJustify::Center);
		CountText->SetColorAndOpacity(FSlateColor(FLinearColor::White));
		CountText->SetShadowColorAndOpacity(FLinearColor::Black);
		CountText->SetShadowOffset(FVector2D(1.0f, 1.0f));
		FSlateFontInfo CountFont = CountText->GetFont();
		CountFont.Size = FMath::Max(24, FormationSlotHeight * 3 / 8);
		CountText->SetFont(CountFont);
		if (UVerticalBoxSlot* CountSlot = InfoBox->AddChildToVerticalBox(CountText))
		{
			FSlateChildSize FillSize;
			FillSize.SizeRule = ESlateSizeRule::Fill;
			FillSize.Value = 1.0f;
			CountSlot->SetSize(FillSize);
			CountSlot->SetHorizontalAlignment(HAlign_Fill);
			CountSlot->SetVerticalAlignment(VAlign_Center);
		}

		Button->SetPresentationWidgets(Icon, NumberText, CountText);

		USizeBox* SlotBox = WidgetTree->ConstructWidget<USizeBox>(
			USizeBox::StaticClass(),
			FName(*FString::Printf(TEXT("ControlGroupSlot_%d"), GroupIndex)));
		if (!SlotBox)
		{
			continue;
		}

		SlotBox->SetWidthOverride(FormationSlotWidth);
		SlotBox->SetHeightOverride(FormationSlotHeight);
		SlotBox->SetContent(Button);
		if (UUniformGridSlot* GridSlot = ControlGroupGrid->AddChildToUniformGrid(SlotBox, 0, SlotIndex))
		{
			GridSlot->SetHorizontalAlignment(HAlign_Fill);
			GridSlot->SetVerticalAlignment(VAlign_Fill);
		}

		SlotBox->SetVisibility(ESlateVisibility::Collapsed);
		ControlGroupButtons.Add(Button);
		ControlGroupSlotBoxes.Add(SlotBox);
	}

	SetVisibility(ESlateVisibility::Collapsed);
}

void URTSFormationListWidget::OnControlGroupsUpdated(const FRTSControlGroupsView& View)
{
	RefreshControlGroups(View);
	OnControlGroupListChanged(View);
}

void URTSFormationListWidget::RefreshControlGroups(const FRTSControlGroupsView& View)
{
	int32 VisibleCardIndex = 0;
	for (int32 ButtonIndex = 0; ButtonIndex < ControlGroupButtons.Num(); ++ButtonIndex)
	{
		URTSControlGroupButton* Button = ControlGroupButtons[ButtonIndex];
		USizeBox* SlotBox = ControlGroupSlotBoxes.IsValidIndex(ButtonIndex) ? ControlGroupSlotBoxes[ButtonIndex] : nullptr;
		if (!Button)
		{
			continue;
		}

		const FRTSControlGroupView* GroupView = View.Groups.FindByPredicate([Button](const FRTSControlGroupView& Candidate)
		{
			return Candidate.GroupIndex == Button->ControlGroupIndex;
		});
		if (GroupView)
		{
			Button->ApplyControlGroupView(*GroupView);
		}

		const bool bShowCard = GroupView && GroupView->bAssigned;
		if (SlotBox)
		{
			SlotBox->SetVisibility(bShowCard ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
			if (bShowCard)
			{
				if (UUniformGridSlot* GridSlot = Cast<UUniformGridSlot>(SlotBox->Slot))
				{
					GridSlot->SetRow(VisibleCardIndex / FMath::Max(1, FormationColumns));
					GridSlot->SetColumn(VisibleCardIndex % FMath::Max(1, FormationColumns));
				}
				++VisibleCardIndex;
			}
		}
	}

	SetVisibility(VisibleCardIndex > 0 ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
}
