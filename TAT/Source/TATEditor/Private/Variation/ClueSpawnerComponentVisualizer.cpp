// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/ClueSpawnerComponentVisualizer.h"

// tat
#include "Variation/ComponentVisualizerTextHelper.h"
#include "Variation/Clues/TATClueSpawner.h"

// ose

// ue4
#include "SceneManagement.h"
#include "Math/Box.h"

void FTATClueSpawnerComponentVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
}

void FTATClueSpawnerComponentVisualizer::DrawVisualizationHUD(const UActorComponent* component, const FViewport* viewport, const FSceneView* view, FCanvas* canvas)
{
   if (const UTATClueSpawnerComponent* spawner = Cast<const UTATClueSpawnerComponent>(component))
   {
      FVector loc = spawner->GetOwner()->GetActorLocation();
      if (VisualizerHelper::FTextDrawer drawer = { loc, view, canvas })
      {
         // TODO add debug hook for more
         FTATClueBucketKey key = spawner->GetClueBucket();
         if (key.PlacementTag.IsValid())
         {
            drawer.AddTextItem(key.PlacementTag.ToString(), FColor::Cyan);
         }

         FGameplayTag requiredLocation = spawner->GetRequiredSourceLocation();
         if (requiredLocation.IsValid())
         {
            drawer.AddTextItem(FString::Printf(TEXT("Required: %s"), *requiredLocation.ToString()), FColor::Orange);
         }

         if (!spawner->GetSceneRequirement().IsNone())
         {
            drawer.AddTextItem(spawner->GetSceneRequirement().ToDebugString(), FColor::Purple);
         }
      }
   }
}
