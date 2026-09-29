// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/BoxComponent.h"
#include "InteractionBoxTrigger.generated.h"

/**
 * Place this under the interactable component to make it easier to interact with this object.
 */
UCLASS(meta = (BlueprintSpawnableComponent))
class OSEINTERACTION_API UInteractionBoxTrigger : public UBoxComponent
{
   GENERATED_BODY()
   
public:

   UInteractionBoxTrigger();
};
