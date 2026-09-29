// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "ComponentVisualizer.h"

class FTATSceneVariantActorSetVisualizer : public FComponentVisualizer
{
public:
   // from FComponentVisualizer
   virtual void DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi) override;
};
