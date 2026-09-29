// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/SphereComponent.h"
#include "InteractionSphereTrigger.generated.h"

/**
 * Place this under the interactable component to make it easier to interact with this object.
 */
UCLASS(meta = (BlueprintSpawnableComponent))

class OSEINTERACTION_API UInteractionSphereTrigger : public USphereComponent
{
   GENERATED_BODY()
   
public:

   UInteractionSphereTrigger();
};
