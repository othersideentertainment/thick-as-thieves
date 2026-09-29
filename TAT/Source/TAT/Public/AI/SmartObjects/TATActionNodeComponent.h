// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/SmartObjects/TATSmartObjectComponent.h"

// ue
#include "CoreMinimal.h"

// self
#include "TATActionNodeComponent.generated.h"

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO: Should we just keep all functionality that would go here action node agnostic and just use UTATSmartObjectComponent?
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

UCLASS(Blueprintable, ClassGroup = Gameplay, meta = (BlueprintSpawnableComponent), config = Game, HideCategories = (Activation, AssetUserData, Collision, Cooking, HLOD, Lighting, LOD, Mobile, Mobility, Navigation, Physics, RayTracing, Rendering, Tags, TextureStreaming), AutoExpandCategories = ("AI|TAT|ActionNode"))
class TAT_API UTATActionNodeComponent : public UTATSmartObjectComponent
{
	GENERATED_BODY()

public:
   UTATActionNodeComponent(const FObjectInitializer& objectInitializer);
   virtual void BeginPlay() override;
   virtual void BeginDestroy() override;

protected:
   UFUNCTION()
   virtual void _HandleSlotEvent(const FSmartObjectEventData& smartObjectEventData);
   int _numberOfSlots { 0 };
};
