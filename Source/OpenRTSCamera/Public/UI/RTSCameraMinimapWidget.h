// Copyright 2024 Jesus Bracho All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTSCameraMinimapWidget.generated.h"

class URTSCamera;

/**
 * URTSCameraMinimapWidget
 * 
 * A widget that visualizes the RTS Camera's Field of View on a minimap.
 * It uses the RTSCamera resolved MapRegion to determine the coordinate system.
 * It handles input to move the camera (JumpTo).
 */
UCLASS(BlueprintType, Blueprintable)
class OPENRTSCAMERA_API URTSCameraMinimapWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URTSCameraMinimapWidget(const FObjectInitializer& ObjectInitializer);

	/** 
	 * Initialize the controller. Tries to find the RTSCamera on the owning player's pawn/view target.
	 */
	UFUNCTION(BlueprintCallable, Category = "RTSCamera|Minimap")
	void InitializeController();

protected:
	/// 小地图视野框的线条绘制宽度
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Minimap")
	float lineWidth = 2.0f;
	
	virtual void NativeConstruct() override;
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const override;

	// --- Input Handling ---
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseButtonUp(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual FReply NativeOnMouseMove(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseEnter(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;
	virtual void NativeOnMouseLeave(const FPointerEvent& InMouseEvent) override;

private:
	/**
	 * @brief       根据相机组件的引用定位并缓存相关依赖资产
	 **/
	void findRTSCamera();

	/**
	 * @brief       响应相机视野更新委托的回调函数，执行 UI 定向失效重绘
	 **/
	void handleMinimapFrustumUpdated();

	/**
	 * @brief       读取 RTS 相机当前使用的地图坐标系。
	 **/
	bool getCurrentBounds(FVector& OutOrigin, FVector& OutExtent) const;

	/** Convert World Location (XY) to Widget Local Coordinates (UV * Size) */
	FVector2D ConvertWorldToWidgetLocal(const FVector2D& WorldPos, const FVector2D& WidgetSize) const;

	/** Convert Widget Local Coordinates to World Location (XY) */
	FVector2D ConvertWidgetLocalToWorld(const FVector2D& LocalPos, const FVector2D& WidgetSize) const;

protected:
	/** @brief 缓存的 RTS 相机组件引用 */
	UPROPERTY(Transient, BlueprintReadOnly, Category = "RTSCamera|Cache")
	TObjectPtr<URTSCamera> cachedRTSCamera;

	/// 状态位：标识玩家当前是否正在通过鼠标在控件上执行位置拖拽
	bool bIsDragging = false;
};
