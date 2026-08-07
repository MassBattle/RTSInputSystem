// Copyright 2024 Jesus Bracho All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "RTSBuildPlacementWireframeComponent.generated.h"

/**
 * Runtime-only building ghost used during placement.
 *
 * The component deliberately renders the authored static mesh as geometry
 * wireframe, leaving the footprint and the surrounding guidance grid visible
 * as independent ground layers.
 */
UCLASS(Transient, NotBlueprintable)
class RTSINPUTSYSTEM_API URTSBuildPlacementWireframeComponent final : public UStaticMeshComponent
{
	GENERATED_BODY()

public:
	void SetPlacementWireframeColor(const FLinearColor& InColor);

protected:
	virtual FPrimitiveSceneProxy* CreateStaticMeshSceneProxy(
		Nanite::FMaterialAudit& NaniteMaterials,
		bool bCreateNanite) override;
};
