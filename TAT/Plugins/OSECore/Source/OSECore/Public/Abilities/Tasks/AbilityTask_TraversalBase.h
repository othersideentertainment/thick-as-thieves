// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/OSEAbilityTask.h"
#include "Traversal/TraversalInterface.h"
#include "AbilityTask_TraversalBase.generated.h"


UCLASS(Abstract)
class OSECORE_API UAbilityTask_TraversalBase : public UOSEAbilityTask
{
   GENERATED_BODY()

public:

   /// Constructor
   UAbilityTask_TraversalBase(const FObjectInitializer& objectInitializer);

protected:

   /// Called to trigger the actual task once the delegates have been set up
   virtual void Activate() override;

   /// End and CleanUp the task - may be called by the task itself or by the task owner if the owner is ending.
   virtual void OnDestroy(bool inOwnerFinished) override;

protected:

   /// Cached character for the avatar actor
   UPROPERTY(Transient)
   class ACharacter* _traversalCharacter;

   /// Cached movement component for the avatar actor
   UPROPERTY(Transient)
   class UCharacterMovementComponent* _traversalMovement;

   /// Cached traversal interface for the avatar actor
   UPROPERTY(Transient)
   TScriptInterface<ITraversalInterface> _traversalInterface;
};
