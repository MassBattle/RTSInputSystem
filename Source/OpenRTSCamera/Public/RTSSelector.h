// Copyright 2024 Jesus Bracho All Rights Reserved.

#pragma once

#include <CoreMinimal.h>
#include "InputAction.h"
#include "InputMappingContext.h"
#include "RTSHUD.h"
#include "RTSSelectable.h"
#include "Components/ActorComponent.h"
#include "RTSSelector.generated.h"

UCLASS(Blueprintable, BlueprintType, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OPENRTSCAMERA_API URTSSelector : public UActorComponent
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

protected:
	virtual void BeginPlay() override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent);
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

private:
	UPROPERTY()
	APlayerController* PlayerController;

	UPROPERTY()
	ARTSHUD* HUD;

	FVector2D SelectionStart;
	FVector2D SelectionEnd;

	bool bIsSelecting;
	bool bSkipCurrentSelectionClick = false;

	void BindInputActions();
	void BindInputMappingContext();
	void CollectComponentDependencyReferences();
};
