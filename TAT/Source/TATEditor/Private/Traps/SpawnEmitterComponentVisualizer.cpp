// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/SpawnEmitterComponentVisualizer.h"

#include "Traps/Old/EmitterComponent_SpawnActor.h"

void FSpawnEmitterComponentVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   const UEmitterComponent_SpawnActor_Old* emitterComponent = Cast<const UEmitterComponent_SpawnActor_Old>(component);
   if (emitterComponent == nullptr)
   {
      return;
   }
   const FSpawnActorEmitterEditorVis& config = emitterComponent->EditorVisualizationConfig;

   const auto offsetSpawnTransform = [&](const FVector& translation) -> FTransform {
      FTransform transform = emitterComponent->GetSpawnTransform();
      FTransform localTransform(translation);
      FTransform::Multiply(&transform, &localTransform, &transform);
      return transform;
   };

   if (config.bDrawArrow)
   {
      DrawDirectionalArrow(pdi, emitterComponent->GetSpawnTransform().ToMatrixNoScale(),
         config.Color, config.ArrowLength, 10, SDPG_World, 1);
   }

   if (config.bDrawSphere)
   {
      DrawWireSphereAutoSides(pdi, offsetSpawnTransform(config.SphereOffset),
         config.Color, config.SphereRadius, SDPG_World, 1);
   }

   if (config.bDrawBox)
   {
      FBox box(config.BoxOffset - config.BoxExtents * 0.5, config.BoxOffset + config.BoxExtents * 0.5);
      DrawWireBox(pdi, emitterComponent->GetSpawnTransform().ToMatrixNoScale(), box, config.Color, SDPG_World, 1);
   }
}
