#include "UI/RTSUnitIconWidget.h"
#include "RTSSelectionSubsystem.h"
#include "UI/RTSTooltipWidget.h"
#include "Interfaces/RTSCommandProgressController.h"
#include "Components/Image.h"
#include "Components/OverlaySlot.h"
#include "Components/ProgressBar.h"
#include "Components/TextBlock.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

namespace
{
	FString GetUnitIconGroupKey(const FRTSUnitData& Data)
	{
		return Data.GroupKey.IsEmpty() ? Data.Name : Data.GroupKey;
	}

	FString BuildUnitProductionLine(const FRTSUnitData& Data)
	{
		TArray<FString> Parts;
		if (Data.bHasProductionCapacity)
		{
			Parts.Add(FString::Printf(TEXT("产能 %d/%d"),
				Data.ProductionBusyLanes,
				Data.ProductionTotalLanes));
			if (Data.ProductionQueuedOrders > 0)
			{
				Parts.Add(FString::Printf(TEXT("等待 %d"), Data.ProductionQueuedOrders));
			}
		}
		if (Data.bHasActivity)
		{
			FString Activity = Data.ActivityLabel.ToString();
			if (Data.ActivityRemainingSeconds > 0.0f)
			{
				Activity += FString::Printf(TEXT(" %.1fs"), Data.ActivityRemainingSeconds);
			}
			if (Data.ActivityQueueCount > 1)
			{
				Activity += FString::Printf(TEXT(" 队列%d"), Data.ActivityQueueCount);
			}
			Parts.Add(MoveTemp(Activity));
		}
		return FString::Join(Parts, TEXT(" · "));
	}

	FString BuildUnitTooltipDescription(const FRTSUnitData& Data)
	{
		TArray<FString> Lines;

		if (!Data.Role.IsEmpty())
		{
			Lines.Add(FString::Printf(TEXT("<RichText.Yellow>%s</>"), *Data.Role));
		}
		if (Data.Count > 1)
		{
			Lines.Add(FString::Printf(TEXT("数量: <RichText.Yellow>%d</>"), Data.Count));
		}
		if (Data.MaxHealth > 0)
		{
			Lines.Add(FString::Printf(TEXT("生命值: <RichText.Green>%.0f / %.0f</>"), Data.Health, Data.MaxHealth));
		}
		if (Data.MaxEnergy > 0)
		{
			Lines.Add(FString::Printf(TEXT("能量: <RichText.Green>%.0f / %.0f</>"), Data.Energy, Data.MaxEnergy));
		}
		if (Data.MaxShield > 0)
		{
			Lines.Add(FString::Printf(TEXT("护盾: <RichText.Green>%.0f / %.0f</>"), Data.Shield, Data.MaxShield));
		}
		const FString ProductionLine = BuildUnitProductionLine(Data);
		if (!ProductionLine.IsEmpty())
		{
			Lines.Add(FString::Printf(TEXT("<RichText.Yellow>%s</>"), *ProductionLine));
		}

		return FString::Join(Lines, TEXT("<n/>"));
	}
}

void URTSUnitIconWidget::NativeOnInitialized()
{
	Super::NativeOnInitialized();

	// Slate asks for the rich tooltip only when it is about to open. This keeps
	// tooltip Blueprint loading and widget construction out of selection frames.
	ToolTipWidgetDelegate.BindDynamic(this, &URTSUnitIconWidget::GetOrCreateTooltipWidget);
}

void URTSUnitIconWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UnitSlotFrame)
	{
		UnitSlotFrame->SetVisibility(ESlateVisibility::Hidden);
	}

	if (!UnitIcon)
	{
		UE_LOG(LogTemp, Warning, TEXT("RTSUnitIconWidget: 'UnitIcon' (Image) is NOT bound! Check your WBP naming. Expecting variable named 'UnitIcon'."));
	}
	else
	{
		UE_LOG(LogTemp, Log, TEXT("RTSUnitIconWidget: NativeConstruct - UnitIcon is bound."));
	}
}

void URTSUnitIconWidget::InitData(const FRTSUnitData& Data, bool bShowIcon, bool bShowBars, bool bShowCount, int32 DesiredIconSize)
{
	// Set Icon
	if (UnitIcon)
	{
		if (UnitSlotFrame)
		{
			const bool bShowCommandFrame = Data.bIsCommandProgressItem;
			UnitSlotFrame->SetRenderOpacity(bShowCommandFrame ? 1.0f : 0.0f);
			UnitSlotFrame->SetVisibility(bShowCommandFrame
				? ESlateVisibility::HitTestInvisible
				: ESlateVisibility::Hidden);
		}

		if (UOverlaySlot* IconSlot = Cast<UOverlaySlot>(UnitIcon->Slot))
		{
			IconSlot->SetPadding(FMargin(0.0f));
			IconSlot->SetHorizontalAlignment(HAlign_Fill);
			IconSlot->SetVerticalAlignment(VAlign_Fill);
		}

		if (!bShowIcon)
		{
			UnitIcon->SetVisibility(ESlateVisibility::Collapsed);
		}
		else
		{
			UnitIcon->SetVisibility(ESlateVisibility::Visible);
			
			if (Data.Icon)
			{
				UnitIcon->SetBrushFromTexture(Data.Icon);
				// Reset color to white (in case it was tinted differently)
				UnitIcon->SetColorAndOpacity(FLinearColor::White);
			}
			else
			{
				UnitIcon->SetColorAndOpacity(FLinearColor::Transparent);
				UnitIcon->SetVisibility(ESlateVisibility::Hidden);
				UE_LOG(LogTemp, Verbose, TEXT("RTSUnitIconWidget: Data.Icon is null for %s. Hiding icon placeholder."), *Data.Name);
			}

			if (DesiredIconSize > 0)
			{
				UnitIcon->SetDesiredSizeOverride(FVector2D(DesiredIconSize, DesiredIconSize));
			}
		}
	}

	UProgressBar* EffectiveActivityBar = ActivityBar;
	if (!EffectiveActivityBar && Data.MaxShield <= 0.0f)
	{
		// Existing Unit.uasset predates ActivityBar.  Reuse its otherwise-unused
		// shield strip so production progress is visible without an asset migration.
		EffectiveActivityBar = ShieldBar;
	}

	// Update Status Bars
	if (bShowBars)
	{
		UpdateBar(HealthBar, Data.Health, Data.MaxHealth);
		UpdateBar(EnergyBar, Data.Energy, Data.MaxEnergy);
		if (ShieldBar != EffectiveActivityBar)
		{
			UpdateBar(ShieldBar, Data.Shield, Data.MaxShield);
		}
	}
	else
	{
		if(HealthBar) HealthBar->SetVisibility(ESlateVisibility::Collapsed);
		if(EnergyBar) EnergyBar->SetVisibility(ESlateVisibility::Collapsed);
		if(ShieldBar) ShieldBar->SetVisibility(ESlateVisibility::Collapsed);
	}

	if (EffectiveActivityBar)
	{
		EffectiveActivityBar->SetPercent(FMath::Clamp(Data.ActivityProgress, 0.0f, 1.0f));
		EffectiveActivityBar->SetVisibility(Data.bHasActivity
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	const FString ProductionLine = BuildUnitProductionLine(Data);
	if (ActivityText)
	{
		ActivityText->SetText(FText::FromString(ProductionLine));
		ActivityText->SetVisibility(ProductionLine.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}

	if (CancelHintText)
	{
		const bool bShowCancel =
			Data.bIsCommandProgressItem
			&& Data.bCanCancelCommandProgressItem;
		CancelHintText->SetVisibility(bShowCancel
			? ESlateVisibility::HitTestInvisible
			: ESlateVisibility::Collapsed);
	}

	if (UnitNameText)
	{
		const FString VisibleName = !ActivityText && !ProductionLine.IsEmpty()
			? FString::Printf(TEXT("%s\n%s"), *Data.Name, *ProductionLine)
			: Data.Name;
		UnitNameText->SetText(FText::FromString(VisibleName));
		UnitNameText->SetVisibility(Data.Name.IsEmpty()
			? ESlateVisibility::Collapsed
			: ESlateVisibility::HitTestInvisible);
	}

	if (CountText)
	{
		if (bShowCount && Data.Count > 1)
		{
			CountText->SetText(FText::AsNumber(Data.Count));
			CountText->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		else
		{
			CountText->SetText(FText::GetEmpty());
			CountText->SetVisibility(ESlateVisibility::Collapsed);
		}
	}

	// Store for Interaction
	StoredData = Data;

	UpdateTooltip(Data);
}

void URTSUnitIconWidget::SetIsActive(bool bActive)
{
	// Visual feedback for Active vs Inactive group
	// Starcraft style: Inactive groups are dimmed.
	SetRenderOpacity(bActive ? 1.0f : 0.3f);
}

void URTSUnitIconWidget::UpdateBar(UProgressBar* Bar, float Current, float Max)
{
	if (!Bar) return;

	if (Max > 0.0f)
	{
		Bar->SetPercent(FMath::Clamp(Current / Max, 0.0f, 1.0f));
		Bar->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		Bar->SetVisibility(ESlateVisibility::Collapsed);
	}
}

TSubclassOf<URTSTooltipWidget> URTSUnitIconWidget::ResolveTooltipClass() const
{
	if (TooltipClass)
	{
		return TooltipClass;
	}

	static TWeakObjectPtr<UClass> CachedTooltipClass;
	if (!CachedTooltipClass.IsValid())
	{
		CachedTooltipClass = LoadClass<URTSTooltipWidget>(
			nullptr,
			TEXT("/Game/UI/HeadUpDisplay/ControlGird/ButtonMessage.ButtonMessage_C")
		);
	}

	TSubclassOf<URTSTooltipWidget> ResolvedClass;
	ResolvedClass = CachedTooltipClass.Get();
	return ResolvedClass;
}

void URTSUnitIconWidget::UpdateTooltip(const FRTSUnitData& Data)
{
	// Unit icon widgets are pooled. If this slot has already been hovered, keep
	// its existing tooltip content current without constructing a new widget.
	if (UnitTooltipWidget)
	{
		UnitTooltipWidget->SetTooltipContent(
			FText::FromString(Data.Name),
			FText::FromString(BuildUnitTooltipDescription(Data)),
			FText::GetEmpty(),
			Data.Icon
		);
	}
}

UWidget* URTSUnitIconWidget::GetOrCreateTooltipWidget()
{
	if (!UnitTooltipWidget)
	{
		if (TSubclassOf<URTSTooltipWidget> ResolvedTooltipClass = ResolveTooltipClass())
		{
			if (APlayerController* PC = GetOwningPlayer())
			{
				UnitTooltipWidget = CreateWidget<URTSTooltipWidget>(PC, ResolvedTooltipClass);
			}
			else if (UWorld* World = GetWorld())
			{
				UnitTooltipWidget = CreateWidget<URTSTooltipWidget>(World, ResolvedTooltipClass);
			}
		}
	}

	if (UnitTooltipWidget)
	{
		UpdateTooltip(StoredData);
	}
	return UnitTooltipWidget;
}

FReply URTSUnitIconWidget::NativeOnPreviewMouseButtonDown(
	const FGeometry& InGeometry,
	const FPointerEvent& InMouseEvent)
{
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton
		&& StoredData.bIsCommandProgressItem)
	{
		UObject* ActionTarget = StoredData.CommandProgressActionTarget.Get();
		if (StoredData.bCanCancelCommandProgressItem
			&& ActionTarget
			&& ActionTarget->Implements<URTSCommandProgressController>())
		{
			IRTSCommandProgressController::
				Execute_RequestCancelCommandProgressItem(
					ActionTarget,
					StoredData.CommandProgressItemId);
		}
		return FReply::Handled();
	}
	return Super::NativeOnPreviewMouseButtonDown(InGeometry, InMouseEvent);
}

FReply URTSUnitIconWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	// Check for Left Click
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		if (StoredData.bIsCommandProgressItem)
		{
			UObject* ActionTarget = StoredData.CommandProgressActionTarget.Get();
			if (StoredData.bCanCancelCommandProgressItem
				&& ActionTarget
				&& ActionTarget->Implements<URTSCommandProgressController>())
			{
				IRTSCommandProgressController::
					Execute_RequestCancelCommandProgressItem(
						ActionTarget,
						StoredData.CommandProgressItemId);
			}
			return FReply::Handled();
		}

		if (APlayerController* PC = GetOwningPlayer())
		{
			if (ULocalPlayer* LP = PC->GetLocalPlayer())
			{
				if (URTSSelectionSubsystem* Subsystem = LP->GetSubsystem<URTSSelectionSubsystem>())
				{
					// --- Starcraft Logic ---
					
					// Shift + Click = Remove (Exclude)
					if (InMouseEvent.IsShiftDown())
					{
						Subsystem->RemoveUnit(StoredData);
						return FReply::Handled();
					}

					// Ctrl + Click = Select Type (Keep only this group)
					if (InMouseEvent.IsControlDown())
					{
						Subsystem->SelectGroup(GetUnitIconGroupKey(StoredData));
						return FReply::Handled();
					}

					// Normal Click = Select This Unit (Exclusive)
					// We need to construct a single selection.
					TArray<AActor*> NewActors;
					TArray<FEntityHandle> NewEntities;
					
					if (StoredData.ActorPtr) NewActors.Add(StoredData.ActorPtr);
					if (StoredData.EntityHandle.Index > 0) NewEntities.Add(StoredData.EntityHandle);
					
					// If Summary Item (Count > 1), normal click usually Selects the GROUP?
					// In SC2: 
					// - Wireframe (List): Click selects unit.
					// - Summary: Click selects ALL of that type (same as Ctrl+Click in Wireframe).
					// If Count > 1, treating as Ctrl+Click (Group Select).
					
					if (StoredData.Count > 1)
					{
						Subsystem->SelectGroup(GetUnitIconGroupKey(StoredData));
					}
					else
					{
						Subsystem->SetSelectedUnits(NewActors, NewEntities, ERTSSelectionModifier::Replace);
					}
					
					return FReply::Handled();
				}
			}
		}
	}

	return Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);
}
