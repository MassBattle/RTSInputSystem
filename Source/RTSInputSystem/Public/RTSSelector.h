// Copyright 2024 Jesus Bracho All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include "GameplayTagContainer.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "RTSHUD.h"
#include "RTSSelectable.h"
#include "Components/ActorComponent.h"
#include "RTSSelector.generated.h"

USTRUCT(BlueprintType)
struct RTSINPUTSYSTEM_API FRTSHashGridSelectionResult
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RTSCamera - Hash Grid Selection")
	FGameplayTag CommandTag;

	UPROPERTY(BlueprintReadOnly, Category = "RTSCamera - Hash Grid Selection")
	FVector WorldLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "RTSCamera - Hash Grid Selection")
	FIntPoint Cell = FIntPoint::ZeroValue;

	UPROPERTY(BlueprintReadOnly, Category = "RTSCamera - Hash Grid Selection")
	float CellSize = 0.0f;

	UPROPERTY(BlueprintReadOnly, Category = "RTSCamera - Hash Grid Selection")
	FVector2D FootprintCells = FVector2D::ZeroVector;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRTSHashGridSelectionCommitted, const FRTSHashGridSelectionResult&, Result);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRTSHashGridSelectionCancelled, FGameplayTag, CommandTag);

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class RTSINPUTSYSTEM_API URTSSelector : public UActorComponent
{
	GENERATED_BODY()

public:
	URTSSelector();

	// BlueprintAssignable allows binding in Blueprints
	DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnActorsSelected, const TArray<AActor*>&, SelectedActors);
	UPROPERTY(BlueprintAssignable)
	FOnActorsSelected OnActorsSelected;

	// BlueprintReadWrite allows access and modification in Blueprints
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputMappingContext* InputMappingContext;

	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* BeginSelection;

	// Action for right clicking to issue a smart command
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "RTSCamera - Inputs")
	UInputAction* IssueCommandAction;

	// --- Targeting State (for Targeted Commands like Move/Attack) ---
	UPROPERTY(BlueprintReadWrite, Category = "RTSCamera - Selection")
	bool bIsTargeting = false;

	UPROPERTY(BlueprintReadWrite, Category = "RTSCamera - Selection")
	FGameplayTag PendingCommandTag;

	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void BeginTargeting(FGameplayTag CommandTag);

	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void CancelTargeting();

	// Input Action handler for Right Click
	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void OnIssueCommand(const FInputActionValue& Value);

	// Function to clear selected actors, can be overridden in Blueprints
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTSCamera - Selection")
	void ClearSelectedActors();

	// Function to handle selected actors, can be overridden in Blueprints
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTSCamera - Selection")
	void HandleSelectedActors(const TArray<AActor*>& NewSelectedActors);
	
	// Function to filter selectable actors, can be overriden in Blueprints
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = "RTSCamera - Selection")
	bool CanSelectActor(AActor* Actor) const;

	// BlueprintCallable to allow calling from Blueprints
	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void OnSelectionStart(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void OnUpdateSelection(const FInputActionValue& Value);

	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Selection")
	void OnSelectionEnd(const FInputActionValue& Value);

	UPROPERTY(BlueprintReadOnly, Category = "RTSCamera - Selection")
	TArray<URTSSelectable*> SelectedActors;

	UPROPERTY(BlueprintAssignable, Category = "RTSCamera - Hash Grid Selection")
	FOnRTSHashGridSelectionCommitted OnHashGridSelectionCommitted;

	UPROPERTY(BlueprintAssignable, Category = "RTSCamera - Hash Grid Selection")
	FOnRTSHashGridSelectionCancelled OnHashGridSelectionCancelled;

	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Hash Grid Selection")
	void BeginHashGridSelection(FGameplayTag CommandTag);

	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Hash Grid Selection")
	void BeginHashGridSelectionWithFootprint(FGameplayTag CommandTag, FVector2D FootprintCells, float CellSize);

	UFUNCTION(BlueprintCallable, Category = "RTSCamera - Hash Grid Selection")
	void CancelHashGridSelection();

	UFUNCTION(BlueprintPure, Category = "RTSCamera - Hash Grid Selection")
	bool IsHashGridSelectionActive() const { return bIsHashGridSelecting; }

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	APlayerController* PlayerController;

	UPROPERTY()
	ARTSHUD* HUD;

	UPROPERTY(Transient)
	TObjectPtr<class UDecalComponent> HashGridSelectionDecalComponent;

	FVector2D SelectionStart;
	FVector2D SelectionEnd;

	bool bIsSelecting;
	bool bSkipCurrentSelectionClick = false;
	bool bIsHashGridSelecting = false;
	FVector2D ActiveHashGridFootprintCells = FVector2D::ZeroVector;
	float ActiveHashGridCellSize = 0.0f;

	void BindInputActions();
	void BindInputMappingContext();
	void CollectComponentDependencyReferences();
	bool ShouldUseHashGridSelectionForCommand(FGameplayTag CommandTag) const;
	bool GetHashGridSelectionResult(FRTSHashGridSelectionResult& OutResult) const;
	bool ProjectHashGridSelectionLocationToGround(const FVector& CandidateLocation, FVector& OutLocation) const;
	FVector SnapHashGridSelectionLocation(const FVector& Location) const;
	FIntPoint GetHashGridCellForLocation(const FVector& Location) const;
	void BeginHashGridSelectionInternal(FGameplayTag CommandTag, FVector2D FootprintCells, float CellSize);
	void UpdateHashGridSelectionPreview();
	void EndHashGridSelectionPreview();
	void CommitHashGridSelection();
};
