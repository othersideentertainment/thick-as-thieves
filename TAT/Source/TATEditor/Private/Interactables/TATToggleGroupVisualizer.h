// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "ComponentVisualizer.h"

class FTATExclusiveToggleRequirementVisualizer : public FComponentVisualizer
{
   // from FComponentVisualizer
   virtual void DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi) override;
};

class FTATExclusiveToggleGroupVisualizer : public FComponentVisualizer
{
   // from FComponentVisualizer
   virtual void DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi) override;
};
