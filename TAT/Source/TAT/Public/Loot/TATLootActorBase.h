// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "GameFramework/Actor.h"

// ose
#include "Interactables/InteractableInterface.h"

#include "TATLootActorBase.generated.h"

class ACharacter;
class ATATLootStashInteractable;

UCLASS(Abstract)
class TAT_API ATATLootActorBase : public AActor, public IInteractableInterface
{
   GENERATED_BODY()

public:
   ATATLootActorBase();

   // From UObject
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // From IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
};
