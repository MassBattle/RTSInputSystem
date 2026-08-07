#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/Texture2D.h"
#include "GameplayTagContainer.h"

#include "RTSSelectable.generated.h"

class UPrimitiveComponent;

UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RTSINPUTSYSTEM_API URTSSelectable : public UActorComponent
{
	GENERATED_BODY()
public:
	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "RTS Selection")
	void OnSelected();

	UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "RTS Selection")
	void OnDeselected();

	/**
	 * Transient cursor preview. The native implementation enables custom depth
	 * on this Actor's visible primitives so the project's outline post process
	 * can draw the mesh silhouette.
	 */
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTS Selection")
	void OnHoverPreviewStarted(int32 StencilValue);

	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTS Selection")
	void OnHoverPreviewEnded();

	// --- Visual Data ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data")
	FString SelectionGroupKey;

	/** Categories consumed by TopSelect and URTSSelectionQueryButton. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data|Quick Selection")
	FGameplayTagContainer SelectionTags;

	/** Actor-backed equivalent of MassBattle's Idle flag. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data|Quick Selection")
	bool bIsIdle = true;

	/** Team used by smart-command hit testing for Actor-backed units without a Mass proxy. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data|Quick Selection", meta = (ClampMin = "0"))
	int32 TeamIndex = 1;

	/**
	 * Actor-side attack range used by the narrow selected-range outline.
	 * Leave at zero to use BuffRange, SelectionRangeOverride, or actor bounds.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data|Selection Feedback", meta = (ClampMin = "0.0", Units = "cm"))
	float AttackRange = 0.0f;

	/**
	 * Aura/buff radius. The visualizer folds it into the same selected-range outline
	 * instead of stacking a second ring over AttackRange.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data|Selection Feedback", meta = (ClampMin = "0.0", Units = "cm"))
	float BuffRange = 0.0f;

	/** Explicit final selected range. Zero uses max(AttackRange, BuffRange). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data|Selection Feedback", meta = (ClampMin = "0.0", Units = "cm"))
	float SelectionRangeOverride = 0.0f;

	/** Runtime task tag used by selected Actor route visualization. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data|Task Feedback")
	FGameplayTag CurrentTaskCommand;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data|Task Feedback")
	FVector CurrentTaskLocation = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data|Task Feedback")
	bool bHasCurrentTaskLocation = false;

	UFUNCTION(BlueprintCallable, Category = "RTS Data|Task Feedback")
	void SetCurrentTaskVisualization(FGameplayTag CommandTag, FVector TargetLocation);

	UFUNCTION(BlueprintCallable, Category = "RTS Data|Task Feedback")
	void ClearCurrentTaskVisualization();

	UFUNCTION(BlueprintPure, Category = "RTS Data|Selection Feedback")
	float GetSelectionFeedbackRange() const;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data")
	UTexture2D* Icon = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data")
	UTexture2D* Avatar = nullptr;

	// --- Status Data (Standard RTS) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data")
	float Health = 100.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data")
	float MaxHealth = 100.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data")
	float Energy = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data")
	float MaxEnergy = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data")
	float Shield = 0.0f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RTS Data")
	float MaxShield = 0.0f;

private:
	struct FHoverPrimitiveState
	{
		TWeakObjectPtr<UPrimitiveComponent> Primitive;
		bool bRenderedCustomDepth = false;
		int32 StencilValue = 0;
	};

	TArray<FHoverPrimitiveState> HoverPrimitiveStates;
	bool bHoverPreviewActive = false;
};
