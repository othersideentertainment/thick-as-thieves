// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "SceneVariantActorSetVisualizer.h"

#include "Variation/SceneVariants/TATSceneVariantActorSet.h"


void FTATSceneVariantActorSetVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   if (component == nullptr)
   {
      return;
   }

   if (const ATATSceneVariantActorSet* actorSet = component->GetOwner<ATATSceneVariantActorSet>())
   {
      const FLinearColor kLineColor = FColor(211, 176, 255); // a light purple to match scene requirements
      for (const AActor* actor : actorSet->Actors)
      {
         if (actor)
         {
            DrawDashedLine(pdi, actorSet->GetActorLocation(), actor->GetActorLocation(), kLineColor, 20, SDPG_World);
         }
      }
   }
}
