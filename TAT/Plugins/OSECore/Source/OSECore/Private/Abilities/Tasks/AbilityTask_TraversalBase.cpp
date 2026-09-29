// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_TraversalBase.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_TraversalBase)


// Constructor
UAbilityTask_TraversalBase::UAbilityTask_TraversalBase(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
   , _traversalCharacter(nullptr)
   , _traversalMovement(nullptr)
   , _traversalInterface(nullptr)
{
}

// Called to trigger the actual task once the delegates have been set up
void UAbilityTask_TraversalBase::Activate()
{
   const FGameplayAbilityActorInfo* actorInfo = Ability->GetCurrentActorInfo();
   _traversalCharacter = Cast<ACharacter>(actorInfo->AvatarActor.Get());
   _traversalMovement = Cast<UCharacterMovementComponent>(actorInfo->MovementComponent.Get());
   _traversalInterface = _traversalCharacter;
   SetWaitingOnAvatar();
};

// End and CleanUp the task - may be called by the task itself or by the task owner if the owner is ending.
void UAbilityTask_TraversalBase::OnDestroy(bool inOwnerFinished)
{
   Super::OnDestroy(inOwnerFinished);
}

