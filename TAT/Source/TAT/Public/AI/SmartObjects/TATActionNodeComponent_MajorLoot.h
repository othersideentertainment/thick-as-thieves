// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AI/Utility/TATUtilityAITargetingGroupInterface.h"
#include "TATActionNodeComponent.h"

// self
#include "TATActionNodeComponent_MajorLoot.generated.h"


UCLASS(Blueprintable, BlueprintType, ClassGroup = Gameplay, meta = (BlueprintSpawnableComponent), config = Game,
   HideCategories = (Activation, AssetUserData, Collision, Cooking, HLOD, Lighting, LOD, Mobile, Mobility, Navigation,
      Physics, RayTracing, Rendering, Tags, TextureStreaming), AutoExpandCategories = ("AI|TAT|ActionNode"))
class TAT_API UTATActionNodeComponent_MajorLoot : public UTATActionNodeComponent 
   , public ITATUtilityAITargetingGroupInterface
{
   GENERATED_BODY()

public:   
   // from ITATUtilityAITargetingGroupInterface
   virtual FGameplayTag GetUtilityAITargetingGroup() const override;
};
