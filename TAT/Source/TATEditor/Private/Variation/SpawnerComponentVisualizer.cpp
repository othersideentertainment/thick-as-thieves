// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/SpawnerComponentVisualizer.h"

// tat
#include "Variation/ComponentVisualizerTextHelper.h"
#include "Variation/TATSpawnerComponent.h"
#include "Variation/TATSpawnerRegistrySubsystem.h"
#include "Variation/TATSpawnTiming.h"

// ose

// ue4
#include "SceneManagement.h"
#include "Math/Box.h"

void FTATSpawnerComponentVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   if (const UTATSpawnerComponent* spawner = Cast<const UTATSpawnerComponent>(component))
   {
      if (const auto* registry = spawner->GetWorld()->GetSubsystem<UTATSpawnerRegistrySubsystem>())
      {
         if (const AActor* dependency = registry->GetEditorVisDependency(spawner))
         {
            const FLinearColor color = spawner->GetParentRequirementType() == ETATSpawnerDependencyType::RequireParentSpawn ? FLinearColor::Blue : FLinearColor::Red;
            pdi->DrawLine(spawner->GetOwner()->GetActorLocation(), dependency->GetActorLocation(), color, SDPG_Foreground);
         }

         registry->ForEachDependentSpawnerActor(spawner->GetOwner(), [spawner, pdi](const AActor* dependsOnThis)
         {
            pdi->DrawLine(spawner->GetOwner()->GetActorLocation(), dependsOnThis->GetActorLocation(), FLinearColor::White, SDPG_Foreground);
         });
      }
   }
}

void FTATSpawnerComponentVisualizer::DrawVisualizationHUD(const UActorComponent* component, const FViewport* viewport, const FSceneView* view, FCanvas* canvas)
{
   if (const UTATSpawnerComponent* spawner = Cast<const UTATSpawnerComponent>(component))
   {
      // For now, skip hud for non-primary spawners on an actor
      if (!spawner->IsPrimarySpawnerForActor())
      {
         return;
      }

      FVector loc = spawner->GetOwner()->GetActorLocation();
      if (VisualizerHelper::FTextDrawer drawer = {loc, view, canvas})
      {
         if (spawner->GetSpawnTiming() != ETATSpawnTiming::Initial)
         {
            TStringBuilder<128> builder;
            builder.Append(TEXTVIEW("Timing: "));
            builder.Append(StaticEnum<ETATSpawnTiming>()->GetDisplayNameTextByValue(static_cast<int64>(spawner->GetSpawnTiming())).ToString());
            drawer.AddTextItem(builder.ToView(), FColor::Red);
         }

         drawer.AddTextItem(spawner->GetSpawnGroupDebugString(), FColor::Cyan);
         if(spawner->GetSpawnBucketAsset())
         {
            drawer.AddTextItem(spawner->GetBucketDebugString(), FColor::Cyan);
         }
         if(spawner->GetClueMode() == ETATSpawnerClueMode::FromSpawnedActor && spawner->GetClueLocationTag().IsValid())
         {
            drawer.AddTextItem(spawner->GetClueLocationTag().ToString(), FColor::Orange);
         }
         
         if(!spawner->GetSceneRequirement().IsNone())
         {
            drawer.AddTextItem(spawner->GetSceneRequirement().ToDebugString(), FColor::Purple);
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
