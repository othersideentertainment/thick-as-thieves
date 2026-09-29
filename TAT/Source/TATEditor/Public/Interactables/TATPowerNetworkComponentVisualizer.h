// (c) 2021-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "ComponentVisualizer.h"

class FTATPowerNetworkComponentVisualizer : public FComponentVisualizer
{
private:
   static void _DrawPowerNetworkRecursive(FPrimitiveDrawInterface* pdi, AActor* actor, AActor* selectedActor, TSet<AActor*>& visitedActors);

   static bool _IsObjectSelected(const UObject* obj);

public:
   // from FComponentVisualizer
   virtual void DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi) override;
   virtual void DrawVisualizationHUD(const UActorComponent* component, const FViewport* viewport, const FSceneView* view, FCanvas* canvas) override;
};
