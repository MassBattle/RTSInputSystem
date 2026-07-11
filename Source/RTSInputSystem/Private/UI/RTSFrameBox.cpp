// Copyright 2024 Winy unq All Rights Reserved.

#include "UI/RTSFrameBox.h"

#include "Materials/MaterialInterface.h"
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SNullWidget.h"

#define LOCTEXT_NAMESPACE "RTSFrameBox"

namespace RTSFrameBoxDefaults
{
	const TCHAR* OuterMaterialPath = TEXT("/Game/UI/HeadUpDisplay/RTSStyle/Materials/M_UI_RTS_FrameOuter.M_UI_RTS_FrameOuter");
	const TCHAR* InnerMaterialPath = TEXT("/Game/UI/HeadUpDisplay/RTSStyle/Materials/M_UI_RTS_FrameInner.M_UI_RTS_FrameInner");
	const TCHAR* FillMaterialPath = TEXT("/Game/UI/HeadUpDisplay/RTSStyle/Materials/M_UI_RTS_PanelFill.M_UI_RTS_PanelFill");
}

URTSFrameBoxSlot::URTSFrameBoxSlot(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, Padding(0.0f)
	, HorizontalAlignment(HAlign_Fill)
	, VerticalAlignment(VAlign_Fill)
{
}

void URTSFrameBoxSlot::SetPadding(FMargin InPadding)
{
	Padding = InPadding;
	ApplySlotProperties();
}

void URTSFrameBoxSlot::SetHorizontalAlignment(EHorizontalAlignment InHorizontalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
	ApplySlotProperties();
}

void URTSFrameBoxSlot::SetVerticalAlignment(EVerticalAlignment InVerticalAlignment)
{
	VerticalAlignment = InVerticalAlignment;
	ApplySlotProperties();
}

void URTSFrameBoxSlot::BuildSlot(TSharedRef<SBorder> InBorder)
{
	Border = InBorder;
	ApplySlotProperties();
	InBorder->SetContent(Content ? Content->TakeWidget() : SNullWidget::NullWidget);
}

void URTSFrameBoxSlot::SynchronizeProperties()
{
	Super::SynchronizeProperties();
	ApplySlotProperties();
}

void URTSFrameBoxSlot::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);
	Border.Reset();
}

void URTSFrameBoxSlot::ApplySlotProperties()
{
	if (TSharedPtr<SBorder> BorderWidget = Border.Pin())
	{
		BorderWidget->SetPadding(Padding);
		BorderWidget->SetHAlign(HorizontalAlignment);
		BorderWidget->SetVAlign(VerticalAlignment);
	}
}

URTSFrameBox::URTSFrameBox(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bIsVariable = false;
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	OuterFrameMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RTSFrameBoxDefaults::OuterMaterialPath));
	InnerFrameMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RTSFrameBoxDefaults::InnerMaterialPath));
	PanelFillMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RTSFrameBoxDefaults::FillMaterialPath));

	OuterTint = FLinearColor::White;
	InnerTint = FLinearColor::White;
	FillTint = FLinearColor::White;
}

void URTSFrameBox::SetFramePadding(FMargin InOuterPadding, FMargin InInnerPadding, FMargin InContentPadding)
{
	OuterPadding = InOuterPadding;
	InnerPadding = InInnerPadding;
	ContentPadding = InContentPadding;
	ApplySlateStyle();
}

void URTSFrameBox::SetContentAlignment(EHorizontalAlignment InHorizontalAlignment, EVerticalAlignment InVerticalAlignment)
{
	HorizontalAlignment = InHorizontalAlignment;
	VerticalAlignment = InVerticalAlignment;
	ApplySlateStyle();
}

void URTSFrameBox::SynchronizeProperties()
{
	Super::SynchronizeProperties();

	RebuildBrushes();
	ApplySlateStyle();
}

void URTSFrameBox::ReleaseSlateResources(bool bReleaseChildren)
{
	Super::ReleaseSlateResources(bReleaseChildren);

	OuterBorder.Reset();
	InnerBorder.Reset();
	FillBorder.Reset();
}

TSharedRef<SWidget> URTSFrameBox::RebuildWidget()
{
	RebuildBrushes();

	FillBorder = SNew(SBorder);
	InnerBorder = SNew(SBorder)
	[
		FillBorder.ToSharedRef()
	];
	OuterBorder = SNew(SBorder)
	[
		InnerBorder.ToSharedRef()
	];

	if (GetChildrenCount() > 0)
	{
		if (URTSFrameBoxSlot* FrameSlot = Cast<URTSFrameBoxSlot>(GetContentSlot()))
		{
			FrameSlot->BuildSlot(FillBorder.ToSharedRef());
		}
		else if (UWidget* ContentWidget = GetContent())
		{
			FillBorder->SetContent(ContentWidget->TakeWidget());
		}
	}

	ApplySlateStyle();
	return OuterBorder.ToSharedRef();
}

UClass* URTSFrameBox::GetSlotClass() const
{
	return URTSFrameBoxSlot::StaticClass();
}

void URTSFrameBox::OnSlotAdded(UPanelSlot* InSlot)
{
	URTSFrameBoxSlot* FrameSlot = CastChecked<URTSFrameBoxSlot>(InSlot);
	FrameSlot->SetPadding(ContentPadding);
	FrameSlot->SetHorizontalAlignment(HorizontalAlignment);
	FrameSlot->SetVerticalAlignment(VerticalAlignment);

	if (FillBorder.IsValid())
	{
		FrameSlot->BuildSlot(FillBorder.ToSharedRef());
		ApplySlateStyle();
	}
}

void URTSFrameBox::OnSlotRemoved(UPanelSlot* InSlot)
{
	if (FillBorder.IsValid())
	{
		FillBorder->SetContent(SNullWidget::NullWidget);
	}
}

#if WITH_EDITOR
const FText URTSFrameBox::GetPaletteCategory()
{
	return LOCTEXT("PaletteCategory", "RTS Input System");
}
#endif

void URTSFrameBox::RebuildBrushes()
{
	EnsureDefaultMaterials();

	OuterBrush = MakeMaterialBoxBrush(OuterFrameMaterial, OuterSlice, OuterTint);
	InnerBrush = MakeMaterialBoxBrush(InnerFrameMaterial, InnerSlice, InnerTint);
	FillBrush = MakeMaterialBoxBrush(PanelFillMaterial, FillSlice, FillTint);
}

void URTSFrameBox::EnsureDefaultMaterials()
{
	if (OuterFrameMaterial.IsNull())
	{
		OuterFrameMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RTSFrameBoxDefaults::OuterMaterialPath));
	}

	if (InnerFrameMaterial.IsNull())
	{
		InnerFrameMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RTSFrameBoxDefaults::InnerMaterialPath));
	}

	if (PanelFillMaterial.IsNull())
	{
		PanelFillMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(RTSFrameBoxDefaults::FillMaterialPath));
	}
}

void URTSFrameBox::ApplySlateStyle()
{
	if (OuterBorder.IsValid())
	{
		OuterBorder->SetBorderImage(&OuterBrush);
		OuterBorder->SetPadding(OuterPadding);
		OuterBorder->SetBorderBackgroundColor(FSlateColor(OuterTint));
		OuterBorder->SetShowEffectWhenDisabled(false);
	}

	if (InnerBorder.IsValid())
	{
		InnerBorder->SetBorderImage(&InnerBrush);
		InnerBorder->SetPadding(InnerPadding);
		InnerBorder->SetBorderBackgroundColor(FSlateColor(InnerTint));
		InnerBorder->SetShowEffectWhenDisabled(false);
	}

	if (FillBorder.IsValid())
	{
		FillBorder->SetBorderImage(&FillBrush);
		FillBorder->SetPadding(ContentPadding);
		FillBorder->SetBorderBackgroundColor(FSlateColor(FillTint));
		FillBorder->SetHAlign(HorizontalAlignment);
		FillBorder->SetVAlign(VerticalAlignment);
		FillBorder->SetShowEffectWhenDisabled(false);
	}

	if (URTSFrameBoxSlot* FrameSlot = Cast<URTSFrameBoxSlot>(GetContentSlot()))
	{
		FrameSlot->SetPadding(ContentPadding);
		FrameSlot->SetHorizontalAlignment(HorizontalAlignment);
		FrameSlot->SetVerticalAlignment(VerticalAlignment);
	}
}

FSlateBrush URTSFrameBox::MakeMaterialBoxBrush(const TSoftObjectPtr<UMaterialInterface>& Material, const FMargin& SlicePixels, const FLinearColor& Tint) const
{
	FSlateBrush Brush;
	Brush.DrawAs = ESlateBrushDrawType::Box;
	Brush.ImageSize = MaterialBaseSize;
	Brush.Margin = SlicePixelsToMargin(SlicePixels);
	Brush.TintColor = FSlateColor(Tint);

	if (!Material.IsNull())
	{
		Brush.SetResourceObject(Material.LoadSynchronous());
	}

	return Brush;
}

FMargin URTSFrameBox::SlicePixelsToMargin(const FMargin& SlicePixels) const
{
	const float Width = FMath::Max(MaterialBaseSize.X, 1.0f);
	const float Height = FMath::Max(MaterialBaseSize.Y, 1.0f);

	return FMargin(
		SlicePixels.Left / Width,
		SlicePixels.Top / Height,
		SlicePixels.Right / Width,
		SlicePixels.Bottom / Height);
}

#undef LOCTEXT_NAMESPACE
