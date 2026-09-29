// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATToggleGroupVisualizer.h"

#include "Interactables/TATExclusiveToggleGroup.h"


void FTATExclusiveToggleRequirementVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   if(const auto* requirement = Cast<UTATExclusiveToggleRequirementComponent>(component))
   {
      if(const AActor* toggleGroup = requirement->GetToggleGoup())
      {
         pdi->DrawLine(toggleGroup->GetActorLocation(), requirement->GetOwner()->GetActorLocation(), FColor::Turquoise, SDPG_Foreground);
      }
   }
}

void FTATExclusiveToggleGroupVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   if(component == nullptr)
   {
      return;
   }

   if(const auto* group = component->GetOwner<ATATExclusiveToggleGroup>())
   {
      for(TWeakObjectPtr<AActor> weakToggle : group->GetEditorVisToggles())
      {
         if(const AActor* toggle = weakToggle.Get())
         {
            pdi->DrawLine(group->GetActorLocation(), toggle->GetActorLocation(), FColor::Turquoise, SDPG_Foreground);
         }
      }
   }
}
