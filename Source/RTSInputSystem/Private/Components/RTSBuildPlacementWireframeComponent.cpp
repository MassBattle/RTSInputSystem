// Copyright 2024 Jesus Bracho All Rights Reserved.

#include "Components/RTSBuildPlacementWireframeComponent.h"

#include "Engine/Engine.h"
#include "Materials/MaterialRenderProxy.h"
#include "StaticMeshResources.h"
#include "StaticMeshSceneProxy.h"

namespace
{
	class FRTSBuildPlacementWireframeSceneProxy final : public FStaticMeshSceneProxy
	{
	public:
		explicit FRTSBuildPlacementWireframeSceneProxy(
			UStaticMeshComponent* Component)
			: FStaticMeshSceneProxy(Component, false)
		{
		}

		virtual void GetDynamicMeshElements(
			const TArray<const FSceneView*>& Views,
			const FSceneViewFamily& ViewFamily,
			uint32 VisibilityMap,
			FMeshElementCollector& Collector) const override
		{
			if (!ViewFamily.EngineShowFlags.StaticMeshes
				|| !GEngine
				|| !GEngine->WireframeMaterial
				|| !RenderData)
			{
				return;
			}

			for (int32 ViewIndex = 0; ViewIndex < Views.Num(); ++ViewIndex)
			{
				const FSceneView* View = Views[ViewIndex];
				if (!View
					|| !IsShown(View)
					|| (VisibilityMap & (1u << ViewIndex)) == 0)
				{
					continue;
				}

				FColoredMaterialRenderProxy* WireframeMaterial =
					new FColoredMaterialRenderProxy(
						GEngine->WireframeMaterial->GetRenderProxy(),
						GetWireframeColor());
				Collector.RegisterOneFrameMaterialProxy(WireframeMaterial);

				const FLODMask LODMask = GetLODMask(View);
				for (int32 LODIndex = ClampedMinLOD;
					LODIndex < RenderData->LODResources.Num();
					++LODIndex)
				{
					if (!LODMask.ContainsLOD(LODIndex)
						|| RenderData->LODResources[LODIndex].Sections.IsEmpty())
					{
						continue;
					}

					for (int32 BatchIndex = 0;
						BatchIndex < GetNumMeshBatches();
						++BatchIndex)
					{
						FMeshBatch& Mesh = Collector.AllocateMesh();
						if (GetWireframeMeshElement(
							LODIndex,
							BatchIndex,
							WireframeMaterial,
							SDPG_World,
							true,
							Mesh))
						{
							Mesh.bCanApplyViewModeOverrides = false;
							Mesh.CastShadow = false;
							Collector.AddMesh(ViewIndex, Mesh);
						}
					}
				}
			}
		}

		virtual FPrimitiveViewRelevance GetViewRelevance(
			const FSceneView* View) const override
		{
			FPrimitiveViewRelevance Relevance =
				FStaticMeshSceneProxy::GetViewRelevance(View);
			Relevance.bStaticRelevance = false;
			Relevance.bDynamicRelevance = Relevance.bDrawRelevance;
			Relevance.bShadowRelevance = false;
			return Relevance;
		}
	};
}

void URTSBuildPlacementWireframeComponent::SetPlacementWireframeColor(
	const FLinearColor& InColor)
{
	FColor NewColor = InColor.GetClamped().ToFColor(true);
	NewColor.A = 255;
	if (!bOverrideWireframeColor || WireframeColorOverride != NewColor)
	{
		bOverrideWireframeColor = true;
		WireframeColorOverride = NewColor;
		MarkRenderStateDirty();
	}
}

FPrimitiveSceneProxy*
URTSBuildPlacementWireframeComponent::CreateStaticMeshSceneProxy(
	Nanite::FMaterialAudit& NaniteMaterials,
	bool bCreateNanite)
{
	return new FRTSBuildPlacementWireframeSceneProxy(this);
}
