// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "ComponentVisualizer.h"

class FSpawnEmitterComponentVisualizer : public FComponentVisualizer
{
public:
   virtual void DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi) override;
};
