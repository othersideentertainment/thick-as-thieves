// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Loot/TATContainerSpawnerComponentVisualizer.h"

// tat
#include "Loot/TATContainerSpawnerComponent.h"

TAutoConsoleVariable<int32> CVarTATLootContainerShowVisualSpawnPoints(
   TEXT("TAT.LootContainerSpawner.DebugVisualSpawnPoints"),
   2,
   TEXT("Whether to show the spawn points for a loot container spawner\n")
   TEXT(" 0 - Don't show\n")
   TEXT(" 1 - Show when editing blueprints, but not in the level viewport\n")
   TEXT(" 2 (default) - Show in the blueprint editor and the level editor\n"));

TAutoConsoleVariable<float> CVarTATLootContainerVisualSpawnPointsAxisLength(
   TEXT("TAT.LootContainerSpawner.DebugVisualSpawnPointsAxisScale"),
   10.0f,
   TEXT("Length used to draw the transform axis for loot container spawn point debug visuals\n")
   TEXT("(default = 10.0)")
);

void FTATContainerSpawnerComponentVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   FTATSpawnerComponentVisualizer::DrawVisualization(component, view, pdi);

   // These cases shouldn't occur, from looking at calling code, but if they do we can handle them
   if (component == nullptr)
   {
      ensureMsgf(false, TEXT("FTATLootContainerSpawnerComponentVisualizer given a null component"));
      return;
   }

   if (component->GetWorld() == nullptr)
   {
      ensureMsgf(false, TEXT("FTATLootContainerSpawnerComponentVisualizer given a component w/o a world"));
      return;
   }

   // Check if this is a preview of the blueprint (the world type for the level viewport will be EWorldType::Editor)
   const bool isPreviewViewport = component->GetWorld()->WorldType == EWorldType::EditorPreview;

   // See if we should show the visualisation based on the world type and the CVar value
   const int32 showVisualSpawnPoints = CVarTATLootContainerShowVisualSpawnPoints.GetValueOnAnyThread();
   if (showVisualSpawnPoints == 0 || (showVisualSpawnPoints == 1 && !isPreviewViewport))
   {
      return;
   }

   if (const UTATContainerSpawnerComponent* containerSpawner = Cast<const UTATContainerSpawnerComponent>(component))
   {
      const float renderScale = CVarTATLootContainerVisualSpawnPointsAxisLength.GetValueOnAnyThread();
      const float thickness = 1.0f;

      for (const FTATContainerSpawnSlotEntry& spawnSlotEntry : containerSpawner->SpawnSlotEntries)
      {
         // Compute spawn transform with relative offset/rotation defined by slot entry
         const FTransform spawnTransform = containerSpawner->GetSpawnTransformWorldSpace(spawnSlotEntry);

         // Draw transform axis
         DrawCoordinateSystem(pdi, spawnTransform.GetLocation(), spawnTransform.GetRotation().Rotator(), renderScale, SDPG_Foreground, thickness);
      }
   }
   else
   {
      // If this fires, it suggests the registration for our visualizers is wrong
      ensureMsgf(false, TEXT("FTATLootContainerSpawnerComponentVisualizer given an unexpected component type: check registration?"));
   }
}
