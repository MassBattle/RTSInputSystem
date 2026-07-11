#include "UI/RTSFormationListWidget.h"

#include "RTSInputPanelSettings.h"
#include "RTSSelectionSubsystem.h"
#include "UI/RTSUnitIconWidget.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/PanelWidget.h"
#include "Components/SizeBox.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

namespace
{
	FString GetFormationListGroupKey(const FRTSUnitData& Data)
	{
		return Data.GroupKey.IsEmpty() ? Data.Name : Data.GroupKey;
	}
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
			if (URTSSelectionSubsystem* Subsystem = LP->GetSubsystem<URTSSelectionSubsystem>())
			{
				Subsystem->OnSelectionChanged.AddUniqueDynamic(this, &URTSFormationListWidget::OnSelectionUpdated);
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
			if (URTSSelectionSubsystem* Subsystem = LP->GetSubsystem<URTSSelectionSubsystem>())
			{
				Subsystem->OnSelectionChanged.RemoveDynamic(this, &URTSFormationListWidget::OnSelectionUpdated);
			}
		}
	}

	Super::NativeDestruct();
}

void URTSFormationListWidget::ApplyFormationSettings()
{
	if (const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>())
	{
		MaxFormationSlots = FMath::Max(1, Settings->FormationListMaxSlots);
		FormationIconSize = FMath::Max(1, Settings->FormationListIconSize);
		FormationSlotGap = FMath::Max(0.0f, Settings->FormationListSlotGap);
	}
}

void URTSFormationListWidget::BuildSlotPool()
{
	if (!FormationSlotContainer)
	{
		UE_LOG(LogTemp, Warning, TEXT("RTSFormationListWidget: FormationSlotContainer is not bound."));
		return;
	}

	if (!FormationIconClass)
	{
		const int32 ChildrenCount = FormationSlotContainer->GetChildrenCount();
		for (int32 ChildIndex = 0; ChildIndex < ChildrenCount; ++ChildIndex)
		{
			if (URTSUnitIconWidget* TemplateIcon = Cast<URTSUnitIconWidget>(FormationSlotContainer->GetChildAt(ChildIndex)))
			{
				FormationIconClass = TemplateIcon->GetClass();
				break;
			}
		}
	}

	if (!FormationIconClass)
	{
		FormationIconClass = LoadClass<URTSUnitIconWidget>(
			nullptr,
			TEXT("/Game/UI/HeadUpDisplay/UnitDetails/Unit.Unit_C")
		);
	}

	FormationSlotContainer->ClearChildren();
	FormationSlots.Reset();

	if (!FormationIconClass || !FormationIconClass->IsChildOf(URTSUnitIconWidget::StaticClass()))
	{
		UE_LOG(LogTemp, Warning, TEXT("RTSFormationListWidget: FormationIconClass is not a URTSUnitIconWidget subclass."));
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < MaxFormationSlots; ++SlotIndex)
	{
		URTSUnitIconWidget* IconWidget = CreateWidget<URTSUnitIconWidget>(this, FormationIconClass);
		if (!IconWidget)
		{
			continue;
		}

		USizeBox* SlotBox = WidgetTree
			? WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), FName(*FString::Printf(TEXT("FormationSlotBox_%02d"), SlotIndex)))
			: NewObject<USizeBox>(this);
		if (!SlotBox)
		{
			continue;
		}

		SlotBox->SetWidthOverride(FormationIconSize);
		SlotBox->SetHeightOverride(FormationIconSize);
		SlotBox->SetContent(IconWidget);
		FormationSlotContainer->AddChild(SlotBox);

		if (UHorizontalBoxSlot* HorizontalSlot = Cast<UHorizontalBoxSlot>(SlotBox->Slot))
		{
			HorizontalSlot->SetPadding(FMargin(0.0f, 0.0f, FormationSlotGap, 0.0f));
			HorizontalSlot->SetHorizontalAlignment(HAlign_Left);
			HorizontalSlot->SetVerticalAlignment(VAlign_Center);
		}

		IconWidget->SetVisibility(ESlateVisibility::Hidden);
		FormationSlots.Add(IconWidget);
	}
}

void URTSFormationListWidget::OnSelectionUpdated(const FRTSSelectionView& View)
{
	RefreshFormationList(View);
	OnFormationListChanged(View);
}

void URTSFormationListWidget::RefreshFormationList(const FRTSSelectionView& View)
{
	if (View.Mode == ERTSSelectionMode::Empty || View.Items.Num() == 0)
	{
		HideSlots();
		SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	HideSlots();

	TSet<FString> SeenGroupKeys;
	int32 VisibleSlotIndex = 0;
	for (const FRTSUnitData& Item : View.Items)
	{
		if (!FormationSlots.IsValidIndex(VisibleSlotIndex))
		{
			break;
		}

		const FString GroupKey = GetFormationListGroupKey(Item);
		if (SeenGroupKeys.Contains(GroupKey))
		{
			continue;
		}
		SeenGroupKeys.Add(GroupKey);

		URTSUnitIconWidget* SlotWidget = FormationSlots[VisibleSlotIndex];
		if (!SlotWidget)
		{
			continue;
		}

		FRTSUnitData FormationData = Item;
		FormationData.Count = FMath::Max(1, Item.Count);
		SlotWidget->InitData(FormationData, true, false, true, FormationIconSize);
		const bool bIsActive = View.ActiveGroupKey.IsEmpty() || GroupKey == View.ActiveGroupKey;
		SlotWidget->SetIsActive(bIsActive);
		SlotWidget->SetVisibility(ESlateVisibility::Visible);
		++VisibleSlotIndex;
	}
}

void URTSFormationListWidget::HideSlots()
{
	for (URTSUnitIconWidget* SlotWidget : FormationSlots)
	{
		if (SlotWidget)
		{
			SlotWidget->SetVisibility(ESlateVisibility::Hidden);
		}
	}
}
