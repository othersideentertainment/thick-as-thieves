// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/QuestSpawnerComponentVisualizer.h"

// tat
#include "Variation/ComponentVisualizerTextHelper.h"
#include "Variation/TATSpawnerRegistrySubsystem.h"
#include "Quests/Spawn/TATQuestActorSpawner.h"

// ose

// ue4
#include "SceneManagement.h"
#include "Math/Box.h"

void FTATQuestSpawnerComponentVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   if (component == nullptr)
   {
      return;
   }

   if (const auto* registry = component->GetWorld()->GetSubsystem<UTATSpawnerRegistrySubsystem>())
   {
      registry->ForEachDependentSpawnerActor(component->GetOwner(), [component, pdi](const AActor* dependsOnThis)
         {
            pdi->DrawLine(component->GetOwner()->GetActorLocation(), dependsOnThis->GetActorLocation(), FLinearColor::White, SDPG_Foreground);
         });
   }
}

void FTATQuestSpawnerComponentVisualizer::DrawVisualizationHUD(const UActorComponent* component, const FViewport* viewport, const FSceneView* view, FCanvas* canvas)
{
   if (const UTATQuestActorSpawnerComponent* spawner = Cast<const UTATQuestActorSpawnerComponent>(component))
   {
      FVector loc = spawner->GetComponentLocation();
      if (VisualizerHelper::FTextDrawer drawer = {loc, view, canvas})
      {
         if (spawner->IsEnabled())
         {
            drawer.AddTextItem(spawner->GetQuestLocationTag().ToString(), FColor::Cyan);
            drawer.AddTextItem(spawner->GetClueLocationName().ToString(), FColor::Cyan);
            if(spawner->GetClueLocationTag().IsValid())
            {
               drawer.AddTextItem(spawner->GetClueLocationTag().ToString(), FColor::Orange);
            }
            if (!spawner->GetSceneRequirement().IsNone())
            {
               drawer.AddTextItem(spawner->GetSceneRequirement().ToDebugString(), FColor::Purple);
            }
         }
         else
         {
            drawer.AddTextItem(TEXT("Disabled"), FColor::Cyan);
         }

         if (const auto* registry = spawner->GetWorld()->GetSubsystem<UTATSpawnerRegistrySubsystem>())
         {
            if (TOptional<float> simulatedSpawnChance = registry->GetLastSimulatedSpawnRate(spawner->GetOwner()))
            {
               drawer.AddTextItem(FString::Printf(TEXT("Last Sim: %.2f%%"), *simulatedSpawnChance), FColor::White);
            }
         }
      }
   }
}
