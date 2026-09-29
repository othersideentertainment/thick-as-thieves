// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "ComponentVisualizer.h"

struct FTATSceneRequirement;

// Avoiding making an uinterface just for editor-only vis
template <typename T>
struct TSceneRequirementVisAdapter
{
   const FTATSceneRequirement* operator()(const UActorComponent* component)
   {
      if (const T* requirementOwner = Cast<T>(component))
      {
         return requirementOwner->FindSceneRequirement();
      }
      return nullptr;
   }
};

class FTATSceneRequirementComponentVisualizer : public FComponentVisualizer
{
public:
   using AdapterFunc = TFunction< const FTATSceneRequirement* (const UActorComponent* component)>;
   FTATSceneRequirementComponentVisualizer(AdapterFunc adaptor)
      : _adaptor(MoveTemp(adaptor))
   {}

   // from FComponentVisualizer
   virtual void DrawVisualization(const UActorComponent* component, const FSceneView* view, FPrimitiveDrawInterface* pdi) override;
   virtual void DrawVisualizationHUD(const UActorComponent* component, const FViewport* viewport, const FSceneView* view, FCanvas* canvas) override;

private:
   AdapterFunc _adaptor;
};

