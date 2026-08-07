#include "RTSSelectable.h"

#include "Components/PrimitiveComponent.h"
#include "GameFramework/Actor.h"

void URTSSelectable::OnHoverPreviewStarted_Implementation(const int32 StencilValue)
{
	if (bHoverPreviewActive)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (!Owner)
	{
		return;
	}

	TArray<UPrimitiveComponent*> Primitives;
	Owner->GetComponents<UPrimitiveComponent>(Primitives);
	HoverPrimitiveStates.Reset();
	HoverPrimitiveStates.Reserve(Primitives.Num());
	for (UPrimitiveComponent* Primitive : Primitives)
	{
		if (!Primitive || !Primitive->IsVisible())
		{
			continue;
		}

		FHoverPrimitiveState& State = HoverPrimitiveStates.AddDefaulted_GetRef();
		State.Primitive = Primitive;
		State.bRenderedCustomDepth = Primitive->bRenderCustomDepth;
		State.StencilValue = Primitive->CustomDepthStencilValue;
		Primitive->SetCustomDepthStencilValue(FMath::Clamp(StencilValue, 0, 255));
		Primitive->SetRenderCustomDepth(true);
	}
	bHoverPreviewActive = true;
}

void URTSSelectable::OnHoverPreviewEnded_Implementation()
{
	if (!bHoverPreviewActive)
	{
		return;
	}

	for (const FHoverPrimitiveState& State : HoverPrimitiveStates)
	{
		if (UPrimitiveComponent* Primitive = State.Primitive.Get())
		{
			Primitive->SetCustomDepthStencilValue(State.StencilValue);
			Primitive->SetRenderCustomDepth(State.bRenderedCustomDepth);
		}
	}
	HoverPrimitiveStates.Reset();
	bHoverPreviewActive = false;
}

void URTSSelectable::SetCurrentTaskVisualization(FGameplayTag CommandTag, FVector TargetLocation)
{
	CurrentTaskCommand = CommandTag;
	CurrentTaskLocation = TargetLocation;
	bHasCurrentTaskLocation = CommandTag.IsValid();
	bIsIdle = !bHasCurrentTaskLocation;
}

void URTSSelectable::ClearCurrentTaskVisualization()
{
	CurrentTaskCommand = FGameplayTag::EmptyTag;
	CurrentTaskLocation = FVector::ZeroVector;
	bHasCurrentTaskLocation = false;
	bIsIdle = true;
}

float URTSSelectable::GetSelectionFeedbackRange() const
{
	if (SelectionRangeOverride > 0.0f)
	{
		return SelectionRangeOverride;
	}
	return FMath::Max(AttackRange, BuffRange);
}
