#include "RTSHUD.h"
#include "RTSSelectionSubsystem.h"
#include "RTSSelectable.h"
#include "FuncLibs/MassBattleFuncLib.h"
#include "RTSSelector.h"
#include "Engine/Canvas.h"
#include "Interfaces/RTSCommandInterface.h"
#include "Data/RTSCommandGridAsset.h"
#include "Engine/Texture2D.h"
#include "GameFramework/PlayerController.h"
#include "MassEntitySubsystem.h"
#include "MassEntityManager.h"
#include "Fragments/SubType.h"

// Constructor implementation: Initializes default values.
ARTSHUD::ARTSHUD()
{
	SelectionBoxColor = FLinearColor(0.0f, 0.78f, 1.0f, 0.95f);
	SelectionBoxFillColor = FLinearColor(0.0f, 0.35f, 0.85f, 0.12f);
	SelectionBoxThickness = 2.0f;
	MinSelectionSizeSq = 1.0f; // 1 pixel threshold as requested
	bIsDrawingSelectionBox = false;
	bIsPerformingSelection = false;
}

// Implementation of the DrawHUD function. It's called every frame to draw the HUD.
void ARTSHUD::DrawHUD()
{
	Super::DrawHUD(); // Call the base class implementation.

	// Draw the selection box if it's active AND large enough to be a box.
	if (bIsDrawingSelectionBox)
	{
		const APlayerController* PC = GetOwningPlayerController();
		if (!PC || !PC->IsInputKeyDown(EKeys::LeftMouseButton))
		{
			bIsDrawingSelectionBox = false;
		}
		else if (FVector2D::DistSquared(SelectionStart, SelectionEnd) > MinSelectionSizeSq)
		{
			DrawSelectionBox(SelectionStart, SelectionEnd);
		}
	}

	// Perform selection actions if required.
	if (bIsPerformingSelection)
	{
		PerformSelection();
        bIsPerformingSelection = false; // CRITICAL: Reset the flag to stop continuous selection
	}
}

// Starts the selection process, setting the initial point and activating the selection flag.
void ARTSHUD::BeginSelection(const FVector2D& StartPoint)
{
	SelectionStart = StartPoint;
	SelectionEnd = StartPoint; // Initialize End to Start to avoid stale data
	bIsDrawingSelectionBox = true;
}

// Updates the current endpoint of the selection box.
void ARTSHUD::UpdateSelection(const FVector2D& EndPoint)
{
	SelectionEnd = EndPoint;
}

// Ends the selection process and triggers the selection logic.
void ARTSHUD::EndSelection()
{
	bIsDrawingSelectionBox = false;
	bIsPerformingSelection = true;
}

// Default implementation of DrawSelectionBox. Draws a rectangle on the HUD.
void ARTSHUD::DrawSelectionBox_Implementation(const FVector2D& StartPoint, const FVector2D& EndPoint)
{
	if (Canvas)
	{
		float MinX = FMath::Min(SelectionStart.X, SelectionEnd.X);
		float MinY = FMath::Min(SelectionStart.Y, SelectionEnd.Y);
		float MaxX = FMath::Max(SelectionStart.X, SelectionEnd.X);
		float MaxY = FMath::Max(SelectionStart.Y, SelectionEnd.Y);
		float Width = MaxX - MinX;
		float Height = MaxY - MinY;

		if (Width > 0 && Height > 0)
		{
			DrawRect(SelectionBoxFillColor, MinX, MinY, Width, Height);
		}

		const FVector2D TopLeft(MinX, MinY);
		const FVector2D TopRight(MaxX, MinY);
		const FVector2D BottomRight(MaxX, MaxY);
		const FVector2D BottomLeft(MinX, MaxY);

		Canvas->K2_DrawLine(TopLeft, TopRight, SelectionBoxThickness, SelectionBoxColor);
		Canvas->K2_DrawLine(TopRight, BottomRight, SelectionBoxThickness, SelectionBoxColor);
		Canvas->K2_DrawLine(BottomRight, BottomLeft, SelectionBoxThickness, SelectionBoxColor);
		Canvas->K2_DrawLine(BottomLeft, TopLeft, SelectionBoxThickness, SelectionBoxColor);

		const float CornerLength = FMath::Clamp(FMath::Min(Width, Height) * 0.18f, 10.0f, 32.0f);
		const float CornerThickness = SelectionBoxThickness + 1.0f;
		const FLinearColor CornerColor(0.55f, 0.95f, 1.0f, 1.0f);

		Canvas->K2_DrawLine(TopLeft, TopLeft + FVector2D(CornerLength, 0.0f), CornerThickness, CornerColor);
		Canvas->K2_DrawLine(TopLeft, TopLeft + FVector2D(0.0f, CornerLength), CornerThickness, CornerColor);
		Canvas->K2_DrawLine(TopRight, TopRight + FVector2D(-CornerLength, 0.0f), CornerThickness, CornerColor);
		Canvas->K2_DrawLine(TopRight, TopRight + FVector2D(0.0f, CornerLength), CornerThickness, CornerColor);
		Canvas->K2_DrawLine(BottomRight, BottomRight + FVector2D(-CornerLength, 0.0f), CornerThickness, CornerColor);
		Canvas->K2_DrawLine(BottomRight, BottomRight + FVector2D(0.0f, -CornerLength), CornerThickness, CornerColor);
		Canvas->K2_DrawLine(BottomLeft, BottomLeft + FVector2D(CornerLength, 0.0f), CornerThickness, CornerColor);
		Canvas->K2_DrawLine(BottomLeft, BottomLeft + FVector2D(0.0f, -CornerLength), CornerThickness, CornerColor);
	}
}

#include "RTSSelectionSubsystem.h"

// Default implementation of PerformSelection. Selects actors within the selection box.
void ARTSHUD::PerformSelection_Implementation()
{
	// 1. Prepare
	ERTSSelectionModifier Modifier = ERTSSelectionModifier::Replace;
    float DragDistSq = FVector2D::DistSquared(SelectionStart, SelectionEnd);

	URTSSelectionSubsystem* SelectionSubsystem = nullptr;
    URTSSelector* SelectorComponent = nullptr;
    APlayerController* PC = GetOwningPlayerController();
	
    if (PC)
	{
        SelectorComponent = PC->FindComponentByClass<URTSSelector>();
		if (const ULocalPlayer* LP = PC->GetLocalPlayer())
		{
			SelectionSubsystem = LP->GetSubsystem<URTSSelectionSubsystem>();
		}

		if (PC->IsInputKeyDown(EKeys::LeftShift) || PC->IsInputKeyDown(EKeys::RightShift))
		{
			Modifier = ERTSSelectionModifier::Add;
		}
	}

    TArray<AActor*> FinalActorSelection;
    TArray<FEntityHandle> FinalMassSelection;

    // 2. SEARCH (Direct & Concurrent)
    
    // A. Actor Path (The primary way to select anything, including Cities now)
    TArray<AActor*> RawActors;
    GetActorsInSelectionRectangle<AActor>(SelectionStart, SelectionEnd, RawActors, false, false);
    for (AActor* Actor : RawActors)
    {
        if (Actor && Actor->FindComponentByClass<URTSSelectable>())
        {
            FinalActorSelection.AddUnique(Actor);
        }
    }

    // B. Entity Path (Soldiers - Mass Battle Standard)
    PerformMassSelection(FinalMassSelection);

	// 5. Toggle Logic (Shift + Single Click = Deselect)
	// ONLY apply toggle if this was a Click (not a Box Drag).
	// Threshold: MinSelectionSizeSq (Synced with Visuals).
	
	if (Modifier == ERTSSelectionModifier::Add && SelectionSubsystem)
	{
		UE_LOG(LogTemp, Log, TEXT("RTSHUD: Shift Action - DragDistSq: %f (Threshold: %f)"), DragDistSq, MinSelectionSizeSq);
		
		if (DragDistSq <= MinSelectionSizeSq)
		{
			// Case A: Single Actor Toggle
			if (FinalActorSelection.Num() == 1 && FinalMassSelection.Num() == 0)
			{
				if (SelectionSubsystem->IsActorSelected(FinalActorSelection[0]))
				{
					Modifier = ERTSSelectionModifier::Remove;
					UE_LOG(LogTemp, Log, TEXT("RTSHUD: Toggling Single Actor OFF (Remove)."));
				}
			}
			// Case B: Single Mass Entity Toggle
			else if (FinalActorSelection.Num() == 0 && FinalMassSelection.Num() == 1)
			{
				if (SelectionSubsystem->IsEntitySelected(FinalMassSelection[0]))
				{
					Modifier = ERTSSelectionModifier::Remove;
					UE_LOG(LogTemp, Log, TEXT("RTSHUD: Toggling Single Entity OFF (Remove)."));
				}
			}
		}
	}

	// 5. Ctrl + Click (Select All of Same Type On Screen)
	if (PC && (PC->IsInputKeyDown(EKeys::LeftControl) || PC->IsInputKeyDown(EKeys::RightControl)))
	{
		// Only apply if it was a Click (not a Box Drag)
		if (DragDistSq <= MinSelectionSizeSq)
		{
			// Strategy: If we clicked a single unit, find all matching units on screen.
			
			// 1. Actor Group Selection
			if (FinalActorSelection.Num() == 1)
			{
				AActor* TemplateActor = FinalActorSelection[0];
				if (TemplateActor)
				{
					UClass* MatchClass = TemplateActor->GetClass();
					
					// Get Viewport Size
					int32 ViewportX, ViewportY;
					PC->GetViewportSize(ViewportX, ViewportY);
					
					// Select All in Viewport
					TArray<AActor*> AllScreenActors;
					GetActorsInSelectionRectangle<AActor>(FVector2D(0,0), FVector2D(ViewportX, ViewportY), AllScreenActors, false, false);
					
					// Filter by Class
					FinalActorSelection.Reset();
					for(AActor* Act : AllScreenActors)
					{
						if (Act && Act->GetClass() == MatchClass && Act->FindComponentByClass<URTSSelectable>())
						{
							FinalActorSelection.Add(Act);
						}
					}
					
					// Force Replace Mode for Group Select
					Modifier = ERTSSelectionModifier::Replace;
					// Clear Mass (prioritize Actor group)
					FinalMassSelection.Reset();
				}
			}
			// 2. Mass Entity Group Selection
			else if (FinalMassSelection.Num() > 0)
			{
				const int32 MatchSubType = GetMassEntitySubtypeIndex(FinalMassSelection[0]);
				if (MatchSubType != INDEX_NONE)
				{
					int32 ViewportX = 0;
					int32 ViewportY = 0;
					PC->GetViewportSize(ViewportX, ViewportY);

					const FVector2D SavedStart = SelectionStart;
					const FVector2D SavedEnd = SelectionEnd;
					SelectionStart = FVector2D(0.0f, 0.0f);
					SelectionEnd = FVector2D(ViewportX, ViewportY);

					TArray<FEntityHandle> AllScreenMass;
					PerformMassSelection(AllScreenMass);

					SelectionStart = SavedStart;
					SelectionEnd = SavedEnd;

					FinalMassSelection.Reset();
					for (const FEntityHandle& Handle : AllScreenMass)
					{
						if (GetMassEntitySubtypeIndex(Handle) == MatchSubType)
						{
							FinalMassSelection.AddUnique(Handle);
						}
					}

					Modifier = ERTSSelectionModifier::Replace;
					FinalActorSelection.Reset();
				}
			}
		}
	}

	// 6. Update Subsystem once, after modifiers have had a chance to rewrite the selection.
	if (SelectionSubsystem)
	{
		SelectionSubsystem->SetSelectedUnits(FinalActorSelection, FinalMassSelection, Modifier);
	}
	
	// Visual Highlighting (Actors)
	if (SelectorComponent)
	{
		const TArray<AActor*>& VisualActors = SelectionSubsystem
			? SelectionSubsystem->GetSelectedActors()
			: FinalActorSelection;

		if (VisualActors.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("RTSHUD: Found %d Selectable Actors."), VisualActors.Num());
			SelectorComponent->HandleSelectedActors(VisualActors);
		}
		else
		{
			// Clear Actor visuals (we either found nothing or found Mass)
			SelectorComponent->HandleSelectedActors(TArray<AActor*>());
			
			if (FinalMassSelection.Num() > 0)
			{
				UE_LOG(LogTemp, Log, TEXT("RTSHUD: Selected %d Mass Entities."), FinalMassSelection.Num());
			}
		}
	}

	bIsPerformingSelection = false;
}

#include "FuncLibs/MassBattleFuncLib.h"
#include "MassBattleStructs.h"

void ARTSHUD::PerformMassSelection(TArray<FEntityHandle>& OutEntities)
{
	OutEntities.Reset();
	
	APlayerController* PC = GetOwningPlayerController();
	if (!PC || !PC->PlayerCameraManager) return;

	// Calculate selection box bounds
	float MinX = FMath::Min(SelectionStart.X, SelectionEnd.X);
    float MinY = FMath::Min(SelectionStart.Y, SelectionEnd.Y);
    float MaxX = FMath::Max(SelectionStart.X, SelectionEnd.X);
    float MaxY = FMath::Max(SelectionStart.Y, SelectionEnd.Y);
    
    float Width = MaxX - MinX;
    float Height = MaxY - MinY;
    float DragDistSq = Width * Width + Height * Height;

    // --- 核心逻辑：统一平截头体选择 (Unified Frustum Selection) ---
    // 无论是点选还是框选，都使用 ViewTraceForAgents 进行后端处理
    bool bIsClick = (DragDistSq < MinSelectionSizeSq);
    
    if (bIsClick)
    {
        // 如果是点选，将单点向四周扩展 1 像素，形成一个微型 2x2 选区
        // 这样可以确保平截头体法线非零，且能利用 Mass 优化的过滤逻辑
        MinX -= 1.0f;
        MaxX += 1.0f;
        MinY -= 1.0f;
        MaxY += 1.0f;
    }

	// 关键：逆时针排列（左上→左下→右下→右上）确保视锥体平面法线朝内
	// 顺时针排列会使法线朝外，导致 PlaneDot 过滤掉框内所有实体
	TArray<FVector2D> ScreenPoints = {
		FVector2D(MinX, MinY),  // 左上
		FVector2D(MinX, MaxY),  // 左下
		FVector2D(MaxX, MaxY),  // 右下
		FVector2D(MaxX, MinY),  // 右上
	};

	FViewTracePoints TracePoints;
	TracePoints.ViewPoint = PC->PlayerCameraManager->GetCameraLocation();

	for (const FVector2D& ScreenPoint : ScreenPoints)
	{
		FVector WorldPos, WorldDirection;
		if (PC->DeprojectScreenPositionToWorld(ScreenPoint.X, ScreenPoint.Y, WorldPos, WorldDirection))
		{
			TracePoints.SelectionPoints.Add(WorldPos + WorldDirection * 100000.0f);
		}
	}

	if (TracePoints.SelectionPoints.Num() == 4)
	{
		bool bHit = false;
		TArray<FTraceResult> Results;
        
		int32 LocalKeepCount = bIsClick ? 1 : -1;
		ESortMode SortMode = bIsClick ? ESortMode::NearToFar : ESortMode::None;

#if WITH_EDITOR
		FTraceDrawDebugConfig DebugCfg;
		DebugCfg.bDrawDebugShape = false;
		DebugCfg.Duration = 1.0f;
		UMassBattleFuncLib::ViewTraceForAgents(this, bHit, Results, LocalKeepCount, TracePoints, false, FVector::ZeroVector, 1.0f, SortMode,
			FVector::ZeroVector, FEntityArray(), FMassBattleQuery(), DebugCfg);
#else
		UMassBattleFuncLib::ViewTraceForAgents(this, bHit, Results, LocalKeepCount, TracePoints, false, FVector::ZeroVector, 1.0f, SortMode);
#endif

		UE_LOG(LogTemp, Warning, TEXT("PerformMassSelection: bHit=%d Results=%d IsClick=%d"), bHit ? 1:0, Results.Num(), bIsClick?1:0);

		if (bHit)
		{
			for (const FTraceResult& Result : Results)
			{
				OutEntities.Add(Result.Entity);
			}
		}
	}
}

int32 ARTSHUD::GetMassEntitySubtypeIndex(const FEntityHandle& Handle) const
{
	if (Handle.Index == 0)
	{
		return INDEX_NONE;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return INDEX_NONE;
	}

	if (UMassEntitySubsystem* MassSys = World->GetSubsystem<UMassEntitySubsystem>())
	{
		FMassEntityManager& EntityManager = MassSys->GetMutableEntityManager();
		const FMassEntityHandle NativeHandle(Handle.Index, Handle.Serial);
		if (EntityManager.IsEntityActive(NativeHandle))
		{
			if (const FSubType* SubType = EntityManager.GetFragmentDataPtr<FSubType>(NativeHandle))
			{
				return SubType->Index;
			}
		}
	}

	return INDEX_NONE;
}
