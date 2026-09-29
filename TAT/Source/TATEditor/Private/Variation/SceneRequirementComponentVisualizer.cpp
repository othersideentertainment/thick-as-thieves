// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/SceneRequirementComponentVisualizer.h"

// tat
#include "Variation/ComponentVisualizerTextHelper.h"
#include "Variation/SceneVariants/TATSceneRequirement.h"

// ose

// ue4
#include "SceneManagement.h"
#include "Math/Box.h"

void FTATSceneRequirementComponentVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
}

void FTATSceneRequirementComponentVisualizer::DrawVisualizationHUD(const UActorComponent* component, const FViewport* viewport, const FSceneView* view, FCanvas* canvas)
{
   const FTATSceneRequirement* requirement  = _adaptor(component);
   if (requirement && !requirement->IsNone())
   {
      FVector loc = component->GetOwner()->GetActorLocation();
      if (VisualizerHelper::FTextDrawer drawer = {loc, view, canvas})
      {
         drawer.AddTextItem(requirement->ToDebugString(), FColor::Purple);
      }
   }
}
