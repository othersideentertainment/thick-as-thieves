// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATPowerSwitchComponentVisualizer.h"

// tat
#include "Interactables/Electrical/TATPowerSource.h"
#include "Interactables/Electrical/TATPowerSwitch.h"

namespace TATPowerSwitchComponentVisualizerUtils
{
   static bool GetPowerSourceAndSwitch(const UActorComponent* component, const UTATPowerSwitch*& powerSwitch, const ATATPowerSource*& powerSource)
   {
      powerSwitch = Cast<UTATPowerSwitch>(component);
      powerSource = powerSwitch ? powerSwitch->GetPowerSource() : nullptr;
      return powerSource && powerSwitch;
   }
}

void FTATPowerSwitchComponentVisualizer::DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi)
{
   const UTATPowerSwitch* powerSwitch = nullptr;
   const ATATPowerSource* powerSource = nullptr;
   if (!TATPowerSwitchComponentVisualizerUtils::GetPowerSourceAndSwitch(component, powerSwitch, powerSource))
   {
      return;
   }

   // Draw line to connected power source
   constexpr float thickness = 1.f;

   const FVector start = powerSwitch->GetOwner()->GetActorLocation();
   const FVector end = powerSource->GetActorLocation();
   pdi->DrawLine(start, end, FLinearColor::Yellow, SDPG_Foreground, thickness);

   // TODO: update TATPowerSource to scrape + cache all referencing switches, and visualize those with lines as well (to provide more context)
}
