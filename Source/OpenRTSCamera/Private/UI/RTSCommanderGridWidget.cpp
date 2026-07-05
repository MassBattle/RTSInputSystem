#include "UI/RTSCommanderGridWidget.h"
#include "Components/UniformGridSlot.h"
#include "Components/InputComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "RTSSelectionSubsystem.h" 
#include "RTSInputPanelSettings.h"
#include "Interfaces/RTSCommandInterface.h" 
#include "RTSSelector.h"
#include "RTSCommandSubsystem.h"
#include "UI/RTSTooltipWidget.h"
#include "HAL/FileManager.h"
#include "ImageUtils.h"
#include "Misc/Paths.h"

namespace
{
	constexpr int32 CommandGridColumns = 5;
	constexpr int32 CommandGridRows = 3;
	constexpr int32 CommandGridSlotCount = CommandGridColumns * CommandGridRows;

	ULocalPlayer* ResolveCommanderGridLocalPlayer(const UUserWidget* Widget)
	{
		if (!Widget)
		{
			return nullptr;
		}

		if (ULocalPlayer* LP = Widget->GetOwningLocalPlayer())
		{
			return LP;
		}

		if (APlayerController* PC = Widget->GetOwningPlayer())
		{
			return PC->GetLocalPlayer();
		}

		if (UWorld* World = Widget->GetWorld())
		{
			return World->GetFirstLocalPlayerFromController();
		}

		return nullptr;
	}

	FKey GetDefaultCommandPanelKey(int32 SlotIndex)
	{
		static const FKey DefaultKeys[] =
		{
			EKeys::Q, EKeys::W, EKeys::E, EKeys::R, EKeys::T,
			EKeys::A, EKeys::S, EKeys::D, EKeys::F, EKeys::G,
			EKeys::Z, EKeys::X, EKeys::C, EKeys::V, EKeys::B
		};

		return SlotIndex >= 0 && SlotIndex < UE_ARRAY_COUNT(DefaultKeys)
			? DefaultKeys[SlotIndex]
			: FKey();
	}

	FKey MakeCommandPanelKeyFromName(const FName KeyName)
	{
		static const TMap<FName, FKey> NamedKeys =
		{
			{ FName(TEXT("Q")), EKeys::Q },
			{ FName(TEXT("W")), EKeys::W },
			{ FName(TEXT("E")), EKeys::E },
			{ FName(TEXT("R")), EKeys::R },
			{ FName(TEXT("T")), EKeys::T },
			{ FName(TEXT("A")), EKeys::A },
			{ FName(TEXT("S")), EKeys::S },
			{ FName(TEXT("D")), EKeys::D },
			{ FName(TEXT("F")), EKeys::F },
			{ FName(TEXT("G")), EKeys::G },
			{ FName(TEXT("Z")), EKeys::Z },
			{ FName(TEXT("X")), EKeys::X },
			{ FName(TEXT("C")), EKeys::C },
			{ FName(TEXT("V")), EKeys::V },
			{ FName(TEXT("B")), EKeys::B },
		};

		if (const FKey* ExplicitKey = NamedKeys.Find(KeyName))
		{
			return *ExplicitKey;
		}

		const FKey ConfiguredKey(KeyName);
		return ConfiguredKey.IsValid() ? ConfiguredKey : FKey();
	}

	FKey GetCommandPanelKey(int32 SlotIndex)
	{
		const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
		if (Settings && Settings->CommandPanelSlots.IsValidIndex(SlotIndex))
		{
			const FName KeyName = Settings->CommandPanelSlots[SlotIndex].Hotkey;
			if (!KeyName.IsNone())
			{
				const FKey ConfiguredKey = MakeCommandPanelKeyFromName(KeyName);
				if (ConfiguredKey.IsValid())
				{
					return ConfiguredKey;
				}
			}
		}

		return GetDefaultCommandPanelKey(SlotIndex);
	}

	bool AreCommandPanelHotkeysEnabled()
	{
		const URTSInputPanelSettings* Settings = GetDefault<URTSInputPanelSettings>();
		return !Settings || Settings->bEnableCommandPanelHotkeys;
	}

	const TCHAR* GetDefaultCommandIconFileName(const FGameplayTag& CommandTag)
	{
		const FName TagName = CommandTag.GetTagName();
		if (TagName == FName(TEXT("RTS.Command.Move")))
		{
			return TEXT("RTS_Command_Move.png");
		}
		if (TagName == FName(TEXT("RTS.Command.Attack")))
		{
			return TEXT("RTS_Command_Attack.png");
		}
		if (TagName == FName(TEXT("RTS.Command.Stop")))
		{
			return TEXT("RTS_Command_Stop.png");
		}
		if (TagName == FName(TEXT("RTS.Command.Hold")))
		{
			return TEXT("RTS_Command_Hold.png");
		}
		if (TagName == FName(TEXT("RTS.Command.Patrol")))
		{
			return TEXT("RTS_Command_Patrol.png");
		}

		const FString TagString = TagName.ToString();
		if (TagString.StartsWith(TEXT("RTS.Command.Build.")))
		{
			return TEXT("RTS_Command_Hold.png");
		}
		if (TagString.StartsWith(TEXT("RTS.Command.Train.")))
		{
			return TEXT("RTS_Command_Move.png");
		}

		return TEXT("RTS_Command_Stop.png");
	}

	UTexture2D* LoadDefaultCommandIcon(const FGameplayTag& CommandTag)
	{
		static TMap<FName, UTexture2D*> IconCache;

		const FName CacheKey(*FString(GetDefaultCommandIconFileName(CommandTag)));
		if (UTexture2D** CachedTexture = IconCache.Find(CacheKey))
		{
			return *CachedTexture;
		}

		const FString IconPath = FPaths::Combine(
			FPaths::ProjectPluginsDir(),
			TEXT("RTSInputSystem"),
			TEXT("Content"),
			TEXT("CommandIcons"),
			TEXT("Source"),
			GetDefaultCommandIconFileName(CommandTag)
		);

		UTexture2D* Texture = nullptr;
		if (IFileManager::Get().FileExists(*IconPath))
		{
			Texture = FImageUtils::ImportFileAsTexture2D(IconPath);
			if (Texture)
			{
				Texture->AddToRoot();
				Texture->SRGB = true;
			}
		}

		IconCache.Add(CacheKey, Texture);
		return Texture;
	}

	FText MakeCommandLabelFromTag(const FGameplayTag& CommandTag)
	{
		FString Label = CommandTag.IsValid()
			? CommandTag.GetTagName().ToString()
			: TEXT("Command");
		Label.RemoveFromStart(TEXT("RTS.Command."));
		Label.ReplaceInline(TEXT("."), TEXT(" "));
		return FText::FromString(Label.IsEmpty() ? TEXT("Command") : Label);
	}

	void EnsureCommandButtonPresentation(URTSCommandButton* Button)
	{
		if (!Button)
		{
			return;
		}

		if (!Button->Icon)
		{
			Button->Icon = LoadDefaultCommandIcon(Button->CommandTag);
		}

		if (Button->DisplayName.IsEmpty())
		{
			Button->DisplayName = MakeCommandLabelFromTag(Button->CommandTag);
		}

		if (Button->Description.IsEmpty())
		{
			Button->Description = FText::FromString(FString::Printf(TEXT("Execute %s."), *Button->DisplayName.ToString()));
		}
	}

}

void URTSCommanderGridWidget::NativePreConstruct()
{
	Super::NativePreConstruct();
}

void URTSCommanderGridWidget::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	InitGridSlots();
}

void URTSCommanderGridWidget::NativeConstruct()
{
	Super::NativeConstruct();

	InitGridSlots();
	
    // 绑定全局通知（模块间解耦的通信枢纽）
	if (ULocalPlayer* LP = ResolveCommanderGridLocalPlayer(this))
	{
		if (URTSSelectionSubsystem* Selection = LP->GetSubsystem<URTSSelectionSubsystem>())
		{
			Selection->OnCommandRefreshRequested.AddUniqueDynamic(this, &URTSCommanderGridWidget::OnActorGridChanged);
			Selection->OnCommandNavigationRequested.AddUniqueDynamic(this, &URTSCommanderGridWidget::OnCommandNavigationRequested);
		}

		// 监听低层级指令系统的导航请求 (二进制导航)
		if (URTSCommandSubsystem* SignalHub = LP->GetSubsystem<URTSCommandSubsystem>())
		{
			CommandNavigationHandle = SignalHub->OnNavigationRequested.AddLambda([this](URTSCommandGridAsset* NewGrid, AActor* Context)
			{
				this->UpdateGrid(NewGrid);
			});
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("RTSCommanderGridWidget: LocalPlayer not found; command navigation binding skipped."));
	}
	
	// If Debug Asset is set, load it immediately for testing
	if (DebugGridAsset)
	{
		// RefreshGrid(DebugGridAsset->GetAllButtons()); // Need better logic here for sparse array
	}

	RegisterCommandPanelHotkeys();
}

void URTSCommanderGridWidget::NativeDestruct()
{
	UnregisterCommandPanelHotkeys();

	if (ULocalPlayer* LP = ResolveCommanderGridLocalPlayer(this))
	{
		if (URTSSelectionSubsystem* Selection = LP->GetSubsystem<URTSSelectionSubsystem>())
		{
			Selection->OnCommandRefreshRequested.RemoveDynamic(this, &URTSCommanderGridWidget::OnActorGridChanged);
			Selection->OnCommandNavigationRequested.RemoveDynamic(this, &URTSCommanderGridWidget::OnCommandNavigationRequested);
		}

		if (URTSCommandSubsystem* SignalHub = LP->GetSubsystem<URTSCommandSubsystem>())
		{
			if (CommandNavigationHandle.IsValid())
			{
				SignalHub->OnNavigationRequested.Remove(CommandNavigationHandle);
				CommandNavigationHandle.Reset();
			}
		}
	}

	Super::NativeDestruct();
}

void URTSCommanderGridWidget::InitGridSlots()
{
	if (!CommandGridPanel)
    {
         UE_LOG(LogTemp, Warning, TEXT("RTSCommanderGridWidget: CommandGridPanel is NULL!"));
         return;
    }

    if (!ButtonParams)
    {
         UE_LOG(LogTemp, Warning, TEXT("RTSCommanderGridWidget: ButtonParams is NULL! Please assign a WBP_CommandButton class in the Widget Blueprint Details."));
         return;
    }

	CommandGridPanel->ClearChildren();
	GridButtons.Empty();

	CommandGridPanel->SetSlotPadding(SlotPadding);
	CommandGridPanel->SetMinDesiredSlotWidth(FMath::Max(1.0f, ButtonSize.X));
	CommandGridPanel->SetMinDesiredSlotHeight(FMath::Max(1.0f, ButtonSize.Y));

	// StarCraft-style command card: 15 slots (3 rows x 5 columns).
	for (int32 Row = 0; Row < CommandGridRows; ++Row)
	{
		for (int32 Col = 0; Col < CommandGridColumns; ++Col)
		{
			URTSCommandButtonWidget* Btn = CreateWidget<URTSCommandButtonWidget>(this, ButtonParams);
			if (Btn)
			{
				UUniformGridSlot* GridSlot = CommandGridPanel->AddChildToUniformGrid(Btn, Row, Col);
				if (GridSlot)
				{
					GridSlot->SetHorizontalAlignment(EHorizontalAlignment::HAlign_Fill);
					GridSlot->SetVerticalAlignment(EVerticalAlignment::VAlign_Fill);
				}

				if (!IsDesignTime())
                {
				    Btn->OnCommandClicked.AddDynamic(this, &URTSCommanderGridWidget::OnGridButtonClicked);
                }
				Btn->Init(nullptr, nullptr, FKey());
				GridButtons.Add(Btn); // Index = Row * CommandGridColumns + Col
			}
		}
	}
}

void URTSCommanderGridWidget::OnSelectionUpdated(const FRTSSelectionView& View)
{
	Super::OnSelectionUpdated(View);
    LastSelectionView = View;

	URTSCommandGridAsset* BaseGrid = nullptr;

	if (ULocalPlayer* LP = ResolveCommanderGridLocalPlayer(this))
	{
		if (URTSSelectionSubsystem* Selection = LP->GetSubsystem<URTSSelectionSubsystem>())
		{
			AActor* ActiveActor = Selection->GetActiveActor();
			ActiveActorPtr = ActiveActor;
			if (ActiveActor && ActiveActor->Implements<URTSCommandInterface>())
			{
				BaseGrid = IRTSCommandInterface::Execute_GetCommandGrid(ActiveActor);
			}
		}
	}

	if (BaseGrid)
	{
		UpdateGrid(BaseGrid);
	}
	else if (View.Items.Num() == 0)
	{
		UpdateGrid(nullptr);
	}
}

void URTSCommanderGridWidget::UpdateGrid(URTSCommandGridAsset* NewGrid)
{
    // 如果是 NULL，即执行 Reset 操作
    CurrentGridAsset = NewGrid;
    
    TArray<URTSCommandButton*> SparseList;
    SparseList.Init(nullptr, CommandGridSlotCount);
    if (NewGrid)
    {
        PopulateSparseButtons(NewGrid, SparseList);
    }
    
    RefreshGrid(SparseList);
    
    if (NewGrid)
    {
        UE_LOG(LogTemp, Log, TEXT("UI-Grid: Set Grid Asset: %s"), *NewGrid->GetName());
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("UI-Grid: Grid Reset (Set NULL)"));
    }
}

void URTSCommanderGridWidget::RefreshVisuals()
{
    if (!CurrentGridAsset.IsValid()) return;

    TArray<URTSCommandButton*> SparseList;
    PopulateSparseButtons(CurrentGridAsset.Get(), SparseList);
    
    for (int32 i = 0; i < CommandGridSlotCount; ++i)
    {
        if (GridButtons.IsValidIndex(i) && GridButtons[i])
        {
            // 增量刷新时必须保留布局决定的快捷键（Q/W/E），否则会被重置为 None
            GridButtons[i]->Init(SparseList[i], ActiveActorPtr.Get(), GetCommandPanelKey(i));
        }
    }
    UE_LOG(LogTemp, Verbose, TEXT("UI-Grid: Visuals Refreshed."));
}

void URTSCommanderGridWidget::PopulateSparseButtons(URTSCommandGridAsset* Grid, TArray<URTSCommandButton*>& OutButtons)
{
    if (!Grid) return;
    OutButtons.Init(nullptr, CommandGridSlotCount);

    // 1. 获取所有按钮（支持虚函数重写，覆盖了单例面板和普通资产面板）
    TArray<URTSCommandButton*> AllButtons = Grid->GetAllButtons();

    // 2. 先尝试放入 PreferredIndex 位置
    TArray<URTSCommandButton*> Untracked;
    for (URTSCommandButton* Btn : AllButtons)
    {
        if (!Btn) continue;
        EnsureCommandButtonPresentation(Btn);

        int32 Idx = Btn->PreferredIndex;
        if (Idx >= 0 && Idx < CommandGridSlotCount && OutButtons[Idx] == nullptr)
        {
            OutButtons[Idx] = Btn;
        }
        else
        {
            Untracked.Add(Btn);
        }
    }

    // 3. 将没有固定位置（或位置冲突）的按钮放入空位
    int32 StartSearch = 0; 
    for (URTSCommandButton* Btn : Untracked)
    {
        for (int32 i = StartSearch; i < CommandGridSlotCount; ++i)
        {
            if (OutButtons[i] == nullptr)
            {
                OutButtons[i] = Btn;
                break;
            }
        }
    }
}

void URTSCommanderGridWidget::RefreshGrid(const TArray<URTSCommandButton*>& Buttons)
{
	if (Buttons.Num() != CommandGridSlotCount) return;

	for (int32 i = 0; i < CommandGridSlotCount; ++i)
	{
		if (GridButtons.IsValidIndex(i) && GridButtons[i])
		{
			GridButtons[i]->Init(Buttons[i], ActiveActorPtr.Get(), GetCommandPanelKey(i));
		}
	}
}

void URTSCommanderGridWidget::OnActorGridChanged()
{
    // 该函数现在转发到 RefreshVisuals
    RefreshVisuals();
}

void URTSCommanderGridWidget::OnCommandNavigationRequested(URTSCommandGridAsset* NewGrid)
{
    // 直接从 Subsystem 拿当前激活 Actor，不走 ActiveActorPtr 中间状态
    if (ULocalPlayer* LP = ResolveCommanderGridLocalPlayer(this))
    {
        if (URTSSelectionSubsystem* Selection = LP->GetSubsystem<URTSSelectionSubsystem>())
        {
            ActiveActorPtr = Selection->GetActiveActor();
        }
    }
    UE_LOG(LogTemp, Warning, TEXT("[Grid] Widget recv Navigation: Grid=%s Actor=%s"),
        NewGrid ? *NewGrid->GetName() : TEXT("NULL"),
        ActiveActorPtr.IsValid() ? *ActiveActorPtr->GetName() : TEXT("NULL"));
    UpdateGrid(NewGrid);
}

#include "Data/RTSCmd_SubMenu.h"

void URTSCommanderGridWidget::OnGridButtonClicked(const FGameplayTag& CommandTag)
{
    // 二进制核心：直接执行
    // 理由：虽然 UI 代理传回的是 Tag，但我们立即将其还原回 Button 对象，
    // 以便执行其包含完整 C++ 逻辑的回调函数（Execute），彻底废除“Actor 查找”链路。
    URTSCommandButton* ClickedData = nullptr;
    for (URTSCommandButtonWidget* BtnWidget : GridButtons)
    {
        if (BtnWidget && BtnWidget->GetVisibility() == ESlateVisibility::Visible)
        {
            if (URTSCommandButton* Data = BtnWidget->GetData())
            {
                if (Data->CommandTag.MatchesTagExact(CommandTag))
                {
                    ClickedData = Data;
                    break;
                }
            }
        }
    }

    if (!ClickedData) return;

    if (ULocalPlayer* LP = GetOwningLocalPlayer())
    {
        if (URTSSelectionSubsystem* Selection = LP->GetSubsystem<URTSSelectionSubsystem>())
        {
            // 对于非针对单位的逻辑（如顾问、科技），Actor 指针可能为空，
            // 但对于“兴奋剂”等单位技能，我们需要传入正确的执行者。
            AActor* ActiveActor = Selection->GetActiveActor();
            const bool bNeedsTarget = ClickedData->TargetType == ERTSCommandTargetType::Location ||
                                     ClickedData->TargetType == ERTSCommandTargetType::LocationOrTarget ||
                                     ClickedData->TargetType == ERTSCommandTargetType::TargetActor;

            // Mass-only 选择下，当前没有 ActiveActor 时不能走“直接 Execute on actor”。
            // 用于目标类命令时，进入 RTSSelector 的瞄点模式；否则直接走全局选中执行。
            if (bNeedsTarget)
            {
                if (APlayerController* PC = GetOwningPlayer())
                {
                    if (URTSSelector* Selector = PC->FindComponentByClass<URTSSelector>())
                    {
                        Selector->BeginTargeting(CommandTag);
                        return;
                    }
                }
                UE_LOG(LogTemp, Warning, TEXT("RTSCommanderGridWidget: Targeting command %s ignored, selector missing."), *CommandTag.ToString());
            }
            else if (ActiveActor && ActiveActor->Implements<URTSCommandInterface>())
            {
                // 战术直达：按钮逻辑自决 (Pure Callback)
                ClickedData->Execute(ActiveActor);
            }
            else
            {
                // Fallback for actor-less selection (pure Mass entities).
                Selection->IssueCommand(CommandTag);
            }
        }
    }
}

// --- Shared Tooltip Implementation ---

void URTSCommanderGridWidget::NotifyButtonHovered(URTSCommandButtonWidget* Btn, URTSCommandButton* Data)
{
    if (!Data) return;

    // Lazy Create
    if (!SharedTooltip && TooltipClass)
    {
        SharedTooltip = CreateWidget<URTSTooltipWidget>(GetOwningPlayer(), TooltipClass);
        if (SharedTooltip)
        {
            SharedTooltip->AddToViewport(100); // High Z-Order
            SharedTooltip->SetVisibility(ESlateVisibility::Collapsed);
            UE_LOG(LogTemp, Log, TEXT("Shared Tooltip Created."));
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("Failed to create Shared Tooltip! Check TooltipClass is valid."));
        }
    }
    else if (!TooltipClass)
    {
         UE_LOG(LogTemp, Warning, TEXT("TooltipClass is NULL in RTSCommanderGridWidget! Please assign WBP_Tooltip."));
    }

    if (SharedTooltip)
    {
        SharedTooltip->UpdateTooltip(Data);
        SharedTooltip->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
        UE_LOG(LogTemp, Verbose, TEXT("Showing Tooltip for: %s"), *Data->DisplayName.ToString());
    }
}

void URTSCommanderGridWidget::NotifyButtonUnhovered(URTSCommandButtonWidget* Btn)
{
    if (SharedTooltip)
    {
        SharedTooltip->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void URTSCommanderGridWidget::RegisterCommandPanelHotkeys()
{
	if (!AreCommandPanelHotkeysEnabled())
	{
		return;
	}

	APlayerController* PC = GetOwningPlayer();
	if (!PC || CommandPanelInputComponent)
	{
		return;
	}

	UWorld* InputWorld = PC->GetWorld();
	if (!InputWorld)
	{
		return;
	}

	CommandPanelInputComponent = NewObject<UInputComponent>(PC);
	if (!CommandPanelInputComponent)
	{
		return;
	}

	CommandPanelInputComponent->Priority = 5;
	CommandPanelInputComponent->bBlockInput = false;
	CommandPanelInputComponent->RegisterComponentWithWorld(InputWorld);

	for (int32 SlotIndex = 0; SlotIndex < CommandGridSlotCount; ++SlotIndex)
	{
		const FKey Hotkey = GetCommandPanelKey(SlotIndex);
		if (!Hotkey.IsValid())
		{
			continue;
		}

		FInputKeyBinding Binding(FInputChord(Hotkey), IE_Pressed);
		Binding.bConsumeInput = true;
		Binding.KeyDelegate.GetDelegateForManualSet().BindLambda([this, SlotIndex]()
		{
			ExecuteCommandPanelSlot(SlotIndex);
		});
		CommandPanelInputComponent->KeyBindings.Add(MoveTemp(Binding));
	}

	FInputKeyBinding TabBinding(FInputChord(EKeys::Tab), IE_Pressed);
	TabBinding.bConsumeInput = true;
	TabBinding.KeyDelegate.GetDelegateForManualSet().BindLambda([this]()
	{
		if (ULocalPlayer* LP = GetOwningLocalPlayer())
		{
			if (URTSSelectionSubsystem* Selection = LP->GetSubsystem<URTSSelectionSubsystem>())
			{
				Selection->CycleGroup();
			}
		}
	});
	CommandPanelInputComponent->KeyBindings.Add(MoveTemp(TabBinding));

	PC->PushInputComponent(CommandPanelInputComponent);
	CommandPanelInputOwner = PC;
}

void URTSCommanderGridWidget::UnregisterCommandPanelHotkeys()
{
	if (!CommandPanelInputComponent)
	{
		return;
	}

	if (CommandPanelInputOwner.IsValid())
	{
		CommandPanelInputOwner->PopInputComponent(CommandPanelInputComponent);
	}

	CommandPanelInputComponent->DestroyComponent();
	CommandPanelInputComponent = nullptr;
	CommandPanelInputOwner.Reset();
}

void URTSCommanderGridWidget::ExecuteCommandPanelSlot(int32 SlotIndex)
{
	if (!GridButtons.IsValidIndex(SlotIndex))
	{
		return;
	}

	URTSCommandButtonWidget* ButtonWidget = GridButtons[SlotIndex];
	if (!ButtonWidget || ButtonWidget->GetVisibility() != ESlateVisibility::Visible)
	{
		return;
	}

	URTSCommandButton* ButtonData = ButtonWidget->GetData();
	if (!ButtonData)
	{
		return;
	}

	OnGridButtonClicked(ButtonData->CommandTag);
}

void URTSCommanderGridWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (SharedTooltip && SharedTooltip->GetVisibility() == ESlateVisibility::SelfHitTestInvisible)
    {
        if (bFixedTooltipAboveGrid)
        {
            // Position it above the CommandGridPanel
            // Use the geometry of this widget to find the screen pos
            FVector2D GridPos = MyGeometry.GetAbsolutePosition();
            
            FVector2D TooltipSize = SharedTooltip->GetDesiredSize();
            if (TooltipSize.IsZero()) TooltipSize = FVector2D(400.0f, 250.0f);

            // Left-Align with the Grid (Standard RTS Style)
            FVector2D FinalPos;
            FinalPos.X = GridPos.X; 
            FinalPos.Y = GridPos.Y - TooltipSize.Y + TooltipYOffset;

            SharedTooltip->SetPositionInViewport(FinalPos);
        }
        else
        {
            // Follow Mouse Logic (Existing)
            FVector2D MousePos;
            if (GetOwningPlayer() && GetOwningPlayer()->GetMousePosition(MousePos.X, MousePos.Y))
            {
                FVector2D ViewportSize = MyGeometry.GetLocalSize();
                if (GEngine && GEngine->GameViewport)
                {
                    GEngine->GameViewport->GetViewportSize(ViewportSize);
                }
                
                FVector2D TooltipSize = SharedTooltip->GetDesiredSize();
                if (TooltipSize.IsZero()) TooltipSize = FVector2D(400.0f, 300.0f);

                FVector2D FinalPos = MousePos;
                
                if (MousePos.X > ViewportSize.X * 0.5f) FinalPos.X -= TooltipSize.X + 10.0f;
                else FinalPos.X += 40.0f;

                if (MousePos.Y > ViewportSize.Y * 0.5f) FinalPos.Y -= TooltipSize.Y + 10.0f;
                else FinalPos.Y += 40.0f;

                SharedTooltip->SetPositionInViewport(FinalPos);
            }
        }
    }
}
