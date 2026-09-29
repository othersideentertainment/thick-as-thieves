// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATPowerSourceComponentVisualizer.h"

// tat
#include "Interactables/Electrical/TATPowerSource.h"
#include "Interactables/Electrical/TATPowerSourceVisComponent.h"
#include "Variation/ComponentVisualizerTextHelper.h"

static TAutoConsoleVariable<bool> CVarDebugDrawChainedPowerSourceTextLabels(
   TEXT("TAT.PowerSource.DebugDrawNames"),
   true,
   TEXT("Controls whether the power source chain debug vis should draw text labels on each power source linked to the selected one."),
   ECVF_Scalability | ECVF_RenderThreadSafe);

namespace TATPowerSourceComponentVisualizer
{
   void ForEachChildOfPowerSource(const UTATPowerSourceVisComponent* powerSourceVisComponent, TFunctionRef<void(const ATATPowerSource* childPowerSource)> function)
   {
      powerSourceVisComponent->TryRefreshChildPowerSources();
      if (const ATATPowerSource* powerSource = powerSourceVisComponent->GetPowerSource())
      {
         const TArray<TWeakObjectPtr<ATATPowerSource>>& childPowerSources = powerSourceVisComponent->GetChildPowerSources();
         for (TWeakObjectPtr<const ATATPowerSource> childPtr : childPowerSources)
         {
            if (const ATATPowerSource* child = childPtr.Get())
            {
               function(child);
            }
         }
      }
   }
}

void FTATPowerSourceComponentVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   if (const UTATPowerSourceVisComponent* powerSourceVisComponent = Cast<UTATPowerSourceVisComponent>(component))
   {
      if (const ATATPowerSource* powerSource = powerSourceVisComponent->GetPowerSource())
      {
         // Visualize parent
         constexpr float thickness = 1.f;
         if (const ATATPowerSource* parent = powerSource->GetParentPowerSource())
         {
            pdi->DrawLine(powerSource->GetActorLocation(), parent->GetActorLocation(), FLinearColor::Yellow, SDPG_Foreground, thickness);
         }

         // Visualize children
         TATPowerSourceComponentVisualizer::ForEachChildOfPowerSource(powerSourceVisComponent, [&](const ATATPowerSource* child)
            {
               pdi->DrawLine(powerSource->GetActorLocation(), child->GetActorLocation(), FLinearColor::Green, SDPG_Foreground, thickness);
            });
      }
   }
}

void FTATPowerSourceComponentVisualizer::DrawVisualizationHUD(const UActorComponent* component, const FViewport* viewport, const FSceneView* view, FCanvas* canvas)
{
   if (const UTATPowerSourceVisComponent* powerSourceVisComponent = Cast<UTATPowerSourceVisComponent>(component))
   {
      if (const ATATPowerSource* powerSource = powerSourceVisComponent->GetPowerSource())
      {
         const bool drawParentChildTextLabels = CVarDebugDrawChainedPowerSourceTextLabels.GetValueOnGameThread();
         
         // Visualize parent
         if (const ATATPowerSource* parent = powerSource->GetParentPowerSource())
         {
            const FVector location = parent->GetActorLocation();
            if (VisualizerHelper::FTextDrawer drawer = { location, view, canvas })
            {
               // Error text when parent references self (always draw this regardless of cvar)
               if (parent == powerSource)
               {
                  drawer.AddTextItem(TEXT("ERROR: PARENT IS SELF"), FColor::Red);
               }
               else if(drawParentChildTextLabels)
               {
                  drawer.AddTextItem(TEXT("PARENT"), FColor::White);
                  drawer.AddTextItem(*parent->GetName(), FColor::White);
               }
            }
         }

         // Visualize children
         if (drawParentChildTextLabels)
         {
            TATPowerSourceComponentVisualizer::ForEachChildOfPowerSource(powerSourceVisComponent, [&](const ATATPowerSource* child)
               {
                  const FVector location = child->GetActorLocation();
                  if (VisualizerHelper::FTextDrawer drawer = { location, view, canvas })
                  {
                     drawer.AddTextItem(TEXT("CHILD"), FColor::White);
                     drawer.AddTextItem(*child->GetName(), FColor::White);
                  }
               });
         }
      }
   }
}
