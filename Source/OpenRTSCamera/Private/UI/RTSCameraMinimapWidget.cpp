// Copyright 2024 Jesus Bracho All Rights Reserved.

#include "UI/RTSCameraMinimapWidget.h"
#include "RTSCamera.h"
#include "OpenRTSCamera.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Rendering/DrawElements.h"

URTSCameraMinimapWidget::URTSCameraMinimapWidget(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	/// 初始化控件可见性，并默认设置绘制线宽
	this->SetVisibility(ESlateVisibility::Visible);
}

void URTSCameraMinimapWidget::NativeConstruct()
{
	Super::NativeConstruct();
	
	/// 锁定交互属性并执行初始控制器搜索
	this->SetVisibility(ESlateVisibility::Visible);
	this->SetIsFocusable(true);
	
	this->InitializeController();
}

void URTSCameraMinimapWidget::InitializeController()
{
	if (APlayerController* playerController = this->GetOwningPlayer())
	{
		playerController->bEnableClickEvents = true;
		playerController->bEnableMouseOverEvents = true;
	}
	this->findRTSCamera();
}

void URTSCameraMinimapWidget::findRTSCamera()
{
	/// 尝试从当前的观察目标或 Pawn 中定位 RTSCamera 组件
	if (!this->cachedRTSCamera)
	{
		APlayerController* playerController = this->GetOwningPlayer();
		if (playerController)
		{
			if (AActor* viewTarget = playerController->GetViewTarget())
			{
				this->cachedRTSCamera = viewTarget->FindComponentByClass<URTSCamera>();
			}
			if (!this->cachedRTSCamera)
			{
				if (APawn* pawn = playerController->GetPawn())
				{
					this->cachedRTSCamera = pawn->FindComponentByClass<URTSCamera>();
				}
			}
		}
	}

	/// 若成功捕获组件，则建立响应式订阅并缓存辅助引用
	if (this->cachedRTSCamera)
	{
		/// 绑定视野更新委托：由相机的“主动推送”驱动 UI 的“局部失效”
		this->cachedRTSCamera->onMinimapFrustumUpdated.RemoveAll(this);
		this->cachedRTSCamera->onMinimapFrustumUpdated.AddUObject(this, &URTSCameraMinimapWidget::handleMinimapFrustumUpdated);

	}
}

bool URTSCameraMinimapWidget::getCurrentBounds(FVector& OutOrigin, FVector& OutExtent) const
{
	if (!this->cachedRTSCamera)
	{
		return false;
	}

	return this->cachedRTSCamera->getResolvedMovementBounds(OutOrigin, OutExtent);
}

void URTSCameraMinimapWidget::handleMinimapFrustumUpdated()
{
	/// 响应式重绘核心：仅在相机通过委托告知数据变动时，才标记 Slate 渲染层失效。
	/// 开发提示：配合 Invalidation Box 使用，可使本控件在静止状态下完全忽略 NativePaint 开销。
	this->Invalidate(EInvalidateWidgetReason::Paint);
}

FVector2D URTSCameraMinimapWidget::ConvertWorldToWidgetLocal(const FVector2D& WorldPos, const FVector2D& WidgetSize) const
{
	/// 将世界坐标系下的点线性映射至小地图控件的局部 0-1 空间，并适配轴向偏移
	FVector boundsOrigin = FVector::ZeroVector;
	FVector boundsExtent = FVector::ZeroVector;
	if (!this->getCurrentBounds(boundsOrigin, boundsExtent) ||
		boundsExtent.X < KINDA_SMALL_NUMBER ||
		boundsExtent.Y < KINDA_SMALL_NUMBER)
	{
		return FVector2D::ZeroVector;
	}

	float normalizedX = (WorldPos.X - (boundsOrigin.X - boundsExtent.X)) / (2.0f * boundsExtent.X);
	float normalizedY = (WorldPos.Y - (boundsOrigin.Y - boundsExtent.Y)) / (2.0f * boundsExtent.Y);
	normalizedX = FMath::Clamp(normalizedX, 0.0f, 1.0f);
	normalizedY = FMath::Clamp(normalizedY, 0.0f, 1.0f);

	return FVector2D(normalizedY * WidgetSize.X, (1.0f - normalizedX) * WidgetSize.Y);
}

FVector2D URTSCameraMinimapWidget::ConvertWidgetLocalToWorld(const FVector2D& LocalPos, const FVector2D& WidgetSize) const
{
	/// 将小地图局部像素坐标反投影回世界地图水平面的 X/Y 坐标
	FVector boundsOrigin = FVector::ZeroVector;
	FVector boundsExtent = FVector::ZeroVector;
	if (!this->getCurrentBounds(boundsOrigin, boundsExtent) ||
		boundsExtent.X < KINDA_SMALL_NUMBER ||
		boundsExtent.Y < KINDA_SMALL_NUMBER ||
		WidgetSize.X <= 0.0f ||
		WidgetSize.Y <= 0.0f)
	{
		return FVector2D::ZeroVector;
	}

	float uParam = FMath::Clamp(LocalPos.X / WidgetSize.X, 0.0f, 1.0f);
	float vParam = FMath::Clamp(LocalPos.Y / WidgetSize.Y, 0.0f, 1.0f);

	float normalizedX = 1.0f - vParam;
	float normalizedY = uParam;

	float worldX = (boundsOrigin.X - boundsExtent.X) + normalizedX * (2.0f * boundsExtent.X);
	float worldY = (boundsOrigin.Y - boundsExtent.Y) + normalizedY * (2.0f * boundsExtent.Y);

	return FVector2D(worldX, worldY);
}

int32 URTSCameraMinimapWidget::NativePaint(
	const FPaintArgs& Args, 
	const FGeometry& AllottedGeometry, 
	const FSlateRect& MyCullingRect, 
	FSlateWindowElementList& OutDrawElements, 
	int32 LayerId, 
	const FWidgetStyle& InWidgetStyle, 
	bool bParentEnabled
) const
{
	/// 执行基础绘制流程。注：如果当前组件没有被 Invalidate，Slate 可能会完全跳过此函数执行。
	int32 maxLayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	FVector2D geometrySize = AllottedGeometry.GetLocalSize();
	if (geometrySize.X < 1.0f || geometrySize.Y < 1.0f)
	{
		return maxLayerId;
	}

	if (!this->cachedRTSCamera)
	{
		const_cast<URTSCameraMinimapWidget*>(this)->findRTSCamera();
	}

	FVector boundsOrigin = FVector::ZeroVector;
	FVector boundsExtent = FVector::ZeroVector;
	if (!this->cachedRTSCamera ||
		!this->getCurrentBounds(boundsOrigin, boundsExtent) ||
		boundsExtent.X < KINDA_SMALL_NUMBER ||
		boundsExtent.Y < KINDA_SMALL_NUMBER)
	{
		return maxLayerId;
	}

	TArray<FVector2D> drawPoints;
	drawPoints.Reserve(5);

	for (int32 i = 0; i < 4; ++i)
	{
		const FVector& worldPt = this->cachedRTSCamera->minimapFrustumPoints[i];
		drawPoints.Add(this->ConvertWorldToWidgetLocal(FVector2D(worldPt.X, worldPt.Y), geometrySize));
	}

	if (drawPoints.Num() > 0)
	{
		/// 彻底消除断言崩溃：显式拷贝首个元素至栈变量。
		/// UE 5.6 严禁直接 Add 容器内部的元素地址，以防扩容时发生非法访问。
		const FVector2D closedPoint = drawPoints[0];
		drawPoints.Add(closedPoint);
	}

	FSlateDrawElement::MakeLines(
		OutDrawElements,
		LayerId + 1,
		AllottedGeometry.ToPaintGeometry(),
		drawPoints,
		ESlateDrawEffect::None,
		FLinearColor(0.0f, 1.0f, 0.55f, 0.95f),
		true,
		this->lineWidth
	);

	return maxLayerId + 1;
}

FReply URTSCameraMinimapWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	/// 响应点击：将屏幕点击直接转化为相机的战略突变
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton)
	{
		this->bIsDragging = true;
		if (this->cachedRTSCamera)
		{
			const FVector2D localPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
			const FVector2D worldPos = this->ConvertWidgetLocalToWorld(localPos, InGeometry.GetLocalSize());
			this->cachedRTSCamera->jumpTo(FVector(worldPos.X, worldPos.Y, 0.0f));
		}
		return FReply::Handled().CaptureMouse(this->TakeWidget());
	}
	return FReply::Unhandled();
}

FReply URTSCameraMinimapWidget::NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	/// 释放拖拽锁
	if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && this->bIsDragging)
	{
		this->bIsDragging = false;
		return FReply::Handled().ReleaseMouseCapture();
	}
	return FReply::Unhandled();
}

FReply URTSCameraMinimapWidget::NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	/// 拖拽追踪：连续更新相机位置
	if (this->bIsDragging && this->HasMouseCapture())
	{
		if (this->cachedRTSCamera)
		{
			const FVector2D localPos = InGeometry.AbsoluteToLocal(InMouseEvent.GetScreenSpacePosition());
			const FVector2D worldPos = this->ConvertWidgetLocalToWorld(localPos, InGeometry.GetLocalSize());
			this->cachedRTSCamera->jumpTo(FVector(worldPos.X, worldPos.Y, 0.0f));
		}
		return FReply::Handled();
	}
	return FReply::Unhandled();
}

void URTSCameraMinimapWidget::NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseEnter(InGeometry, InMouseEvent);
}

void URTSCameraMinimapWidget::NativeOnMouseLeave(const FPointerEvent& InMouseEvent)
{
	Super::NativeOnMouseLeave(InMouseEvent);
}
