// Copyright 2024 Jesus Bracho All Rights Reserved.

#include "RTSSelector.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "GameFramework/PlayerController.h"
#include "RTSInputPanelSettings.h"
#include "RTSSelectable.h"
#include "RTSSelectionSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "UObject/ConstructorHelpers.h"
#include "Components/DecalComponent.h"
#include "Components/MassBattleAgentComponent.h"
#include "Materials/MaterialInterface.h"

// Sets default values for this component's properties
URTSSelector::URTSSelector(): PlayerController(nullptr), HUD(nullptr), bIsSelecting(false)
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// Add defaults for input actions
	static ConstructorHelpers::FObjectFinder<UInputAction>
		BeginSelectionActionFinder(TEXT("/RTSInputSystem/Inputs/BeginSelection"));
	static ConstructorHelpers::FObjectFinder<UInputMappingContext>
		InputMappingContextFinder(TEXT("/RTSInputSystem/Inputs/RTSInputSystemInputs"));
	this->BeginSelection = BeginSelectionActionFinder.Object;
	this->InputMappingContext = InputMappingContextFinder.Object;

	static ConstructorHelpers::FObjectFinder<UInputAction>
		IssueCommandActionFinder(TEXT("/RTSInputSystem/Inputs/IssueCommand"));
	if (IssueCommandActionFinder.Succeeded())
	{
		this->IssueCommandAction = IssueCommandActionFinder.Object;
	}
}


// Called when the game starts
void URTSSelector::BeginPlay()
{
	Super::BeginPlay();

	const auto NetMode = this->GetNetMode();
	if (NetMode != NM_DedicatedServer)
	{
		this->CollectComponentDependencyReferences();
		this->BindInputMappingContext();
		this->BindInputActions();
		OnActorsSelected.AddDynamic(this, &URTSSelector::HandleSelectedActors);
	}
}

void URTSSelector::HandleSelectedActors_Implementation(const TArray<AActor*>& NewSelectedActors)
{
	// Convert NewSelectedActors to a set for efficient lookup
	TSet<AActor*> FilteredSelectedActors;
	for (const auto& Actor : NewSelectedActors)
	{
		if (Actor && this->CanSelectActor(Actor))
		{
			FilteredSelectedActors.Add(Actor);
		}
	}

	// Iterate over currently selected actors
	for (const auto& Selected : this->SelectedActors)
	{
		// Check if the actor is not in the new selection
		if (!FilteredSelectedActors.Contains(Selected->GetOwner()))
		{
			// Call OnDeselected for actors that are no longer selected
			Selected->OnDeselected();
		}
	}

	// Clear the current selection
	ClearSelectedActors();
    
	// Add new selected actors and call OnSelected
	for (const auto& Actor : NewSelectedActors)
	{
		if (URTSSelectable* SelectableComponent = Actor->FindComponentByClass<URTSSelectable>())
		{
			this->SelectedActors.Add(SelectableComponent);
			SelectableComponent->OnSelected();
		}
	}
}

void URTSSelector::ClearSelectedActors_Implementation()
{
	this->SelectedActors.Empty();
}

// Called every frame
void URTSSelector::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsTargeting && bIsHashGridSelecting)
	{
		UpdateHashGridSelectionPreview();
	}
}

void URTSSelector::CollectComponentDependencyReferences()
{
	if (const auto PlayerControllerRef = UGameplayStatics::GetPlayerController(this->GetWorld(), 0))
	{
		this->PlayerController = PlayerControllerRef;
		this->HUD = Cast<ARTSHUD>(PlayerControllerRef->GetHUD());
		if (this->HUD)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RTSSelector] HUD found OK: %s"), *this->HUD->GetClass()->GetName());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[RTSSelector] HUD cast FAILED! PC HUD class is: %s"),
				PlayerControllerRef->GetHUD() ? *PlayerControllerRef->GetHUD()->GetClass()->GetName() : TEXT("NULL"));
		}
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("USelector is not attached to a PlayerController."));
	}
}

void URTSSelector::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	if (const auto InputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		InputComponent->BindAction(this->BeginSelection, ETriggerEvent::Started, this, &URTSSelector::OnSelectionStart);
		InputComponent->BindAction(this->BeginSelection, ETriggerEvent::Completed, this, &URTSSelector::OnSelectionEnd);
		if (this->IssueCommandAction)
		{
			InputComponent->BindAction(this->IssueCommandAction, ETriggerEvent::Started, this, &URTSSelector::OnIssueCommand);
		}
	}
}

void URTSSelector::BindInputActions()
{
	if (const auto EnhancedInputComponent = Cast<UEnhancedInputComponent>(this->PlayerController->InputComponent))
	{
		EnhancedInputComponent->BindAction(
			this->BeginSelection,
			ETriggerEvent::Started,
			this,
			&URTSSelector::OnSelectionStart
		);

		EnhancedInputComponent->BindAction(
			this->BeginSelection,
			ETriggerEvent::Triggered,
			this,
			&URTSSelector::OnUpdateSelection
		);

		EnhancedInputComponent->BindAction(
			this->BeginSelection,
			ETriggerEvent::Completed,
			this,
			&URTSSelector::OnSelectionEnd
		);

		if (!this->IssueCommandAction)
		{
			// Dynamically load it in case the CDO failed to find it during editor startup
			this->IssueCommandAction = Cast<UInputAction>(StaticLoadObject(UInputAction::StaticClass(), nullptr, TEXT("/Script/EnhancedInput.InputAction'/RTSInputSystem/Inputs/IssueCommand.IssueCommand'")));
		}

		if (this->IssueCommandAction)
		{
			UE_LOG(LogTemp, Warning, TEXT("[RTSSelector] IssueCommandAction BOUND SUCCESSFULLY!"));
			EnhancedInputComponent->BindAction(
				this->IssueCommandAction,
				ETriggerEvent::Started,
				this,
				&URTSSelector::OnIssueCommand
			);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[RTSSelector] IssueCommandAction is NULL! Ensure it exists at /RTSInputSystem/Inputs/IssueCommand"));
		}
	}
}

void URTSSelector::BindInputMappingContext()
{
	if (PlayerController && PlayerController->GetLocalPlayer())
	{
		if (const auto Input = PlayerController->GetLocalPlayer()->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			PlayerController->bShowMouseCursor = true;

			// Check if the context is already bound to prevent double binding
			if (!Input->HasMappingContext(this->InputMappingContext))
			{
				Input->ClearAllMappings();
				Input->AddMappingContext(this->InputMappingContext, 0);
			}
		}
	}
}

void URTSSelector::BeginTargeting(FGameplayTag CommandTag)
{
	if (ShouldUseHashGridSelectionForCommand(CommandTag))
	{
		BeginHashGridSelection(CommandTag);
		return;
	}

	EndHashGridSelectionPreview();
	bIsTargeting = true;
	PendingCommandTag = CommandTag;
	// Optional: Change mouse cursor to crosshair here
	if (PlayerController)
	{
		PlayerController->CurrentMouseCursor = EMouseCursor::Crosshairs;
	}
}

void URTSSelector::CancelTargeting()
{
	if (bIsHashGridSelecting)
	{
		CancelHashGridSelection();
		return;
	}

	EndHashGridSelectionPreview();
	bIsTargeting = false;
	PendingCommandTag = FGameplayTag::EmptyTag;
	if (PlayerController)
	{
		PlayerController->CurrentMouseCursor = EMouseCursor::Default;
	}
}

void URTSSelector::OnIssueCommand(const FInputActionValue& Value)
{
	if (!PlayerController) return;

	FHitResult Hit;
	PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit);

	UE_LOG(LogTemp, Warning, TEXT("[RTSSelector] OnIssueCommand TRIGGERED! bBlockingHit: %d, Location: %s"), Hit.bBlockingHit, *Hit.Location.ToString());

	if (bIsTargeting)
	{
		if (bIsHashGridSelecting)
		{
			CancelHashGridSelection();
			return;
		}

		if (Hit.bBlockingHit)
		{
			if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
			{
				if (URTSSelectionSubsystem* SelectionSubsystem = LocalPlayer->GetSubsystem<URTSSelectionSubsystem>())
				{
					const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Attack"), false);
					if (PendingCommandTag == AttackTag && Hit.GetActor())
					{
						SelectionSubsystem->IssueCommandWithTarget(PendingCommandTag, Hit.GetActor());
					}
					else
					{
						SelectionSubsystem->IssueCommandWithLocation(PendingCommandTag, Hit.Location);
					}
				}
			}
		}

		CancelTargeting(); // Right click also resolves targeting command.
		return;
	}

	if (Hit.bBlockingHit)
	{
		if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
		{
			if (URTSSelectionSubsystem* SelectionSubsystem = LocalPlayer->GetSubsystem<URTSSelectionSubsystem>())
			{
				AActor* HitActor = Hit.GetActor();
				const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Attack"), false);
				if (HitActor && HitActor->FindComponentByClass<UMassBattleAgentComponent>())
				{
					SelectionSubsystem->IssueCommandWithTarget(AttackTag, HitActor);
				}
				else
				{
					SelectionSubsystem->IssueCommandWithLocation(FGameplayTag::RequestGameplayTag(FName("RTS.Command.Move"), false), Hit.Location);
				}
			}
		}
	}

	CancelTargeting(); // Right click cancels any active targeting
}

void URTSSelector::OnSelectionStart(const FInputActionValue& Value)
{
	if (bIsTargeting)
	{
		bSkipCurrentSelectionClick = true;

		if (bIsHashGridSelecting)
		{
			CommitHashGridSelection();
			return;
		}

		if (PlayerController)
		{
			FHitResult Hit;
			PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit);
			if (Hit.bBlockingHit)
			{
				if (ULocalPlayer* LocalPlayer = PlayerController->GetLocalPlayer())
				{
					if (URTSSelectionSubsystem* SelectionSubsystem = LocalPlayer->GetSubsystem<URTSSelectionSubsystem>())
					{
						// Issue command with location or target actor
						// For now, always use location, as Mass Agents can move to a location.
						const FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("RTS.Command.Attack"), false);
						if (PendingCommandTag == AttackTag && Hit.GetActor())
						{
							SelectionSubsystem->IssueCommandWithTarget(PendingCommandTag, Hit.GetActor());
						}
						else
						{
							SelectionSubsystem->IssueCommandWithLocation(PendingCommandTag, Hit.Location);
						}
					}
				}
			}
		}

		CancelTargeting();
		return;
	}

	bSkipCurrentSelectionClick = false;
	FVector2D MousePosition;
	PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y);
	HUD->BeginSelection(MousePosition);
}

void URTSSelector::OnUpdateSelection(const FInputActionValue& Value)
{
	if (bSkipCurrentSelectionClick) return;
	FVector2D MousePosition;
	PlayerController->GetMousePosition(MousePosition.X, MousePosition.Y);
	SelectionEnd = MousePosition;
	HUD->UpdateSelection(SelectionEnd);
}

void URTSSelector::OnSelectionEnd(const FInputActionValue& Value)
{
	if (bSkipCurrentSelectionClick) return;
	// Call PerformSelection on the HUD to execute selection logic
	HUD->EndSelection();
}

bool URTSSelector::CanSelectActor_Implementation(AActor *Actor) const {
	return true;
}

void URTSSelector::BeginHashGridSelection(FGameplayTag CommandTag)
{
	const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
	const FVector2D FootprintCells = Settings ? Settings->HashGridSelectionFootprintCells : FVector2D(3.0f, 3.0f);
	const float CellSize = Settings ? Settings->HashGridCellSize : 400.0f;
	BeginHashGridSelectionInternal(CommandTag, FootprintCells, CellSize);
}

void URTSSelector::BeginHashGridSelectionWithFootprint(FGameplayTag CommandTag, FVector2D FootprintCells, float CellSize)
{
	BeginHashGridSelectionInternal(CommandTag, FootprintCells, CellSize);
}

void URTSSelector::CancelHashGridSelection()
{
	if (!bIsHashGridSelecting)
	{
		return;
	}

	const FGameplayTag CancelledCommandTag = PendingCommandTag;
	bIsHashGridSelecting = false;
	EndHashGridSelectionPreview();
	bIsTargeting = false;
	PendingCommandTag = FGameplayTag::EmptyTag;
	ActiveHashGridFootprintCells = FVector2D::ZeroVector;
	ActiveHashGridCellSize = 0.0f;

	if (PlayerController)
	{
		PlayerController->CurrentMouseCursor = EMouseCursor::Default;
	}

	OnHashGridSelectionCancelled.Broadcast(CancelledCommandTag);
}

bool URTSSelector::ShouldUseHashGridSelectionForCommand(FGameplayTag CommandTag) const
{
	return CommandTag.IsValid() && CommandTag.GetTagName().ToString().StartsWith(TEXT("RTS.Command.Build."));
}

FVector URTSSelector::SnapHashGridSelectionLocation(const FVector& Location) const
{
	const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
	const float CellSize = ActiveHashGridCellSize > KINDA_SMALL_NUMBER
		? ActiveHashGridCellSize
		: (Settings ? Settings->HashGridCellSize : 400.0f);

	const bool bShouldSnap = !Settings || Settings->bSnapHashGridSelectionToGrid;
	if (!bShouldSnap || CellSize <= KINDA_SMALL_NUMBER)
	{
		return Location;
	}

	FVector Snapped = Location;
	Snapped.X = FMath::GridSnap(Snapped.X, CellSize);
	Snapped.Y = FMath::GridSnap(Snapped.Y, CellSize);
	return Snapped;
}

FIntPoint URTSSelector::GetHashGridCellForLocation(const FVector& Location) const
{
	const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
	const float CellSize = ActiveHashGridCellSize > KINDA_SMALL_NUMBER
		? ActiveHashGridCellSize
		: (Settings ? Settings->HashGridCellSize : 400.0f);

	if (CellSize <= KINDA_SMALL_NUMBER)
	{
		return FIntPoint::ZeroValue;
	}

	return FIntPoint(
		FMath::FloorToInt(Location.X / CellSize),
		FMath::FloorToInt(Location.Y / CellSize)
	);
}

bool URTSSelector::ProjectHashGridSelectionLocationToGround(const FVector& CandidateLocation, FVector& OutLocation) const
{
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}

	const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
	const float TraceHalfHeight = Settings ? Settings->HashGridSelectionTraceHalfHeight : 50000.0f;
	const FVector TraceStart(CandidateLocation.X, CandidateLocation.Y, CandidateLocation.Z + TraceHalfHeight);
	const FVector TraceEnd(CandidateLocation.X, CandidateLocation.Y, CandidateLocation.Z - TraceHalfHeight);

	FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(RTSHashGridSelectionTrace), true);
	if (PlayerController)
	{
		if (APawn* Pawn = PlayerController->GetPawn())
		{
			QueryParams.AddIgnoredActor(Pawn);
		}
	}

	FHitResult Hit;
	if (World->LineTraceSingleByChannel(Hit, TraceStart, TraceEnd, ECC_Visibility, QueryParams))
	{
		OutLocation = Hit.Location;
		return true;
	}

	OutLocation = CandidateLocation;
	return true;
}

bool URTSSelector::GetHashGridSelectionResult(FRTSHashGridSelectionResult& OutResult) const
{
	if (!PlayerController)
	{
		return false;
	}

	FHitResult Hit;
	PlayerController->GetHitResultUnderCursor(ECC_Visibility, false, Hit);
	if (!Hit.bBlockingHit)
	{
		return false;
	}

	FVector WorldLocation = FVector::ZeroVector;
	if (!ProjectHashGridSelectionLocationToGround(SnapHashGridSelectionLocation(Hit.Location), WorldLocation))
	{
		return false;
	}

	OutResult.CommandTag = PendingCommandTag;
	OutResult.WorldLocation = WorldLocation;
	OutResult.Cell = GetHashGridCellForLocation(WorldLocation);
	OutResult.CellSize = ActiveHashGridCellSize;
	OutResult.FootprintCells = ActiveHashGridFootprintCells;
	return true;
}

void URTSSelector::BeginHashGridSelectionInternal(FGameplayTag CommandTag, FVector2D FootprintCells, float CellSize)
{
	EndHashGridSelectionPreview();

	bIsTargeting = true;
	bIsHashGridSelecting = true;
	PendingCommandTag = CommandTag;
	ActiveHashGridFootprintCells = FVector2D(
		FMath::Max(1.0f, FootprintCells.X),
		FMath::Max(1.0f, FootprintCells.Y)
	);
	ActiveHashGridCellSize = FMath::Max(1.0f, CellSize);

	if (PlayerController)
	{
		PlayerController->CurrentMouseCursor = EMouseCursor::Crosshairs;
	}

	const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
	if (Settings && !Settings->bEnableHashGridSelectionPreview)
	{
		return;
	}

	if (!HashGridSelectionDecalComponent)
	{
		UObject* DecalOuter = GetOwner() ? static_cast<UObject*>(GetOwner()) : static_cast<UObject*>(this);
		HashGridSelectionDecalComponent = NewObject<UDecalComponent>(DecalOuter, TEXT("RTSHashGridSelectionDecal"));
		if (HashGridSelectionDecalComponent)
		{
			HashGridSelectionDecalComponent->RegisterComponentWithWorld(GetWorld());
			HashGridSelectionDecalComponent->SetWorldRotation(FRotator(-90.0f, 0.0f, 0.0f));
			HashGridSelectionDecalComponent->SetVisibility(false);
			HashGridSelectionDecalComponent->FadeScreenSize = 0.0f;
		}
	}

	if (!HashGridSelectionDecalComponent)
	{
		return;
	}

	if (!HashGridSelectionDecalComponent->GetDecalMaterial())
	{
		UMaterialInterface* DecalMaterial = Settings ? Settings->HashGridSelectionDecalMaterial.LoadSynchronous() : nullptr;
		if (!DecalMaterial)
		{
			DecalMaterial = Cast<UMaterialInterface>(StaticLoadObject(
				UMaterialInterface::StaticClass(),
				nullptr,
				TEXT("/RTSInputSystem/Feedback/M_RTSBuildPlacementGrid.M_RTSBuildPlacementGrid")
			));
		}

		if (DecalMaterial)
		{
			HashGridSelectionDecalComponent->SetDecalMaterial(DecalMaterial);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("RTSSelector: Missing hash-grid selection decal material."));
		}
	}

	const float DecalDepth = Settings ? Settings->HashGridSelectionDecalDepth : 4096.0f;
	HashGridSelectionDecalComponent->DecalSize = FVector(
		FMath::Max(1.0f, DecalDepth),
		FMath::Max(1.0f, ActiveHashGridFootprintCells.X * ActiveHashGridCellSize),
		FMath::Max(1.0f, ActiveHashGridFootprintCells.Y * ActiveHashGridCellSize)
	);

	UpdateHashGridSelectionPreview();
}

void URTSSelector::UpdateHashGridSelectionPreview()
{
	if (!HashGridSelectionDecalComponent)
	{
		return;
	}

	FRTSHashGridSelectionResult Result;
	if (GetHashGridSelectionResult(Result))
	{
		HashGridSelectionDecalComponent->SetWorldLocation(Result.WorldLocation + FVector(0.0f, 0.0f, 8.0f));
		HashGridSelectionDecalComponent->SetWorldRotation(FRotator(-90.0f, 0.0f, 0.0f));
		HashGridSelectionDecalComponent->SetVisibility(HashGridSelectionDecalComponent->GetDecalMaterial() != nullptr);
	}
	else
	{
		HashGridSelectionDecalComponent->SetVisibility(false);
	}
}

void URTSSelector::EndHashGridSelectionPreview()
{
	if (HashGridSelectionDecalComponent)
	{
		HashGridSelectionDecalComponent->SetVisibility(false);
	}
}

void URTSSelector::CommitHashGridSelection()
{
	FRTSHashGridSelectionResult Result;
	const bool bHasResult = GetHashGridSelectionResult(Result);
	const FGameplayTag CommandToIssue = PendingCommandTag;

	bIsHashGridSelecting = false;
	EndHashGridSelectionPreview();
	bIsTargeting = false;
	PendingCommandTag = FGameplayTag::EmptyTag;
	ActiveHashGridFootprintCells = FVector2D::ZeroVector;
	ActiveHashGridCellSize = 0.0f;

	if (PlayerController)
	{
		PlayerController->CurrentMouseCursor = EMouseCursor::Default;
	}

	if (!bHasResult)
	{
		return;
	}

	OnHashGridSelectionCommitted.Broadcast(Result);

	if (CommandToIssue.IsValid())
	{
		if (ULocalPlayer* LocalPlayer = PlayerController ? PlayerController->GetLocalPlayer() : nullptr)
		{
			if (URTSSelectionSubsystem* SelectionSubsystem = LocalPlayer->GetSubsystem<URTSSelectionSubsystem>())
			{
				SelectionSubsystem->IssueCommandWithLocation(CommandToIssue, Result.WorldLocation);
			}
		}
	}
}
