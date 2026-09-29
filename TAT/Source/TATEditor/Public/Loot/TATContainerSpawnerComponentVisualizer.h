// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Variation/SpawnerComponentVisualizer.h"

// ue
#include "ComponentVisualizer.h"

class FTATContainerSpawnerComponentVisualizer : public FTATSpawnerComponentVisualizer
{
public:
   virtual void DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi) override;
};
