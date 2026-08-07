#include "UI/RTSSelectionQueryButton.h"

#include "RTSSelectionSubsystem.h"
#include "Engine/LocalPlayer.h"
#include "Framework/Application/SlateApplication.h"

URTSSelectionQueryButton::URTSSelectionQueryButton()
{
	SelectionQuery.bIncludeMassEntities = true;
	SelectionQuery.bIncludeActorUnits = false;
}

void URTSSelectionQueryButton::OnWidgetRebuilt()
{
	Super::OnWidgetRebuilt();
	OnClicked.AddUniqueDynamic(this, &URTSSelectionQueryButton::HandleClicked);
}

void URTSSelectionQueryButton::ReleaseSlateResources(bool bReleaseChildren)
{
	OnClicked.RemoveDynamic(this, &URTSSelectionQueryButton::HandleClicked);
	Super::ReleaseSlateResources(bReleaseChildren);
}

FRTSSelectionQuery URTSSelectionQueryButton::GetEffectiveSelectionQuery() const
{
	return SelectionQuery;
}

FRTSSelectionQuery URTSSelectAllArmyButton::GetEffectiveSelectionQuery() const
{
	FRTSSelectionQuery Query = SelectionQuery;
	Query.RequiredSelectionTag = FGameplayTag::RequestGameplayTag(FName(TEXT("RTS.Selection.Army")), false);
	Query.ExcludedSelectionTag = FGameplayTag::RequestGameplayTag(FName(TEXT("RTS.Selection.Structure")), false);
	Query.bIdleOnly = false;
	return Query;
}

FRTSSelectionQuery URTSSelectAllIdleArmyButton::GetEffectiveSelectionQuery() const
{
	FRTSSelectionQuery Query = SelectionQuery;
	Query.RequiredSelectionTag = FGameplayTag::RequestGameplayTag(FName(TEXT("RTS.Selection.Army")), false);
	Query.ExcludedSelectionTag = FGameplayTag::RequestGameplayTag(FName(TEXT("RTS.Selection.Structure")), false);
	Query.bIdleOnly = true;
	return Query;
}

int32 URTSSelectionQueryButton::ExecuteSelectionQuery()
{
	ULocalPlayer* LocalPlayer = GetOwningLocalPlayer();
	URTSSelectionSubsystem* Selection = LocalPlayer ? LocalPlayer->GetSubsystem<URTSSelectionSubsystem>() : nullptr;
	if (!Selection)
	{
		OnSelectionQueryExecuted.Broadcast(0);
		return 0;
	}

	ERTSSelectionModifier EffectiveModifier = SelectionModifier;
	if (bUseKeyboardModifiers && FSlateApplication::IsInitialized())
	{
		const FModifierKeysState Modifiers = FSlateApplication::Get().GetModifierKeys();
		if (Modifiers.IsShiftDown())
		{
			EffectiveModifier = ERTSSelectionModifier::Add;
		}
		else if (Modifiers.IsControlDown())
		{
			EffectiveModifier = ERTSSelectionModifier::Remove;
		}
	}

	const int32 SelectedCount = Selection->SelectUnitsByQuery(GetEffectiveSelectionQuery(), EffectiveModifier);
	OnSelectionQueryExecuted.Broadcast(SelectedCount);
	return SelectedCount;
}

void URTSSelectionQueryButton::HandleClicked()
{
	ExecuteSelectionQuery();
}
