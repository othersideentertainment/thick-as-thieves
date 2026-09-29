// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Utility/UtilityAIState.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "AI/OSEAIController.h"
#include "AI/Utility/UtilityAIComponent.h"
#include "AI/Utility/UtilityAITokenOwnerSmartObject.h"
#include "Character/OSECharacterBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAIState)

// ue4

///////////////////////////////////////////////////////////////////
///        UUtilityAIStateBase
///////////////////////////////////////////////////////////////////

UUtilityAIStateBase::UUtilityAIStateBase()
   : Super()
{
}

UWorld* UUtilityAIStateBase::GetWorld() const
{
   return _utilityAIComponent ? _utilityAIComponent->GetWorld() : nullptr;
}

void UUtilityAIStateBase::Init(UUtilityAIComponent& utilityAIComponent)
{
   _utilityAIComponent = &utilityAIComponent;
   BP_Init();
}

void UUtilityAIStateBase::Reset()
{
   BP_Reset();
}

void UUtilityAIStateBase::Enter()
{
   _mostRecentStartTime = GetWorld()->GetTimeSeconds();
   BP_Enter();
}

void UUtilityAIStateBase::Exit()
{
   _mostRecentEndTime = GetWorld()->GetTimeSeconds();
   AOSEAIController* aiController = GetController();

   // skip doing work on exit if our controller is being cleaned up
   if (IsValid(aiController))
   {
      BP_Exit();
   }
}

void UUtilityAIStateBase::Tick(float deltaTime)
{
   BP_Tick(deltaTime);
}

AOSEAIController* UUtilityAIStateBase::GetController() const
{
   return _utilityAIComponent ? _utilityAIComponent->GetAIController() : nullptr;
}

AOSECharacterBase* UUtilityAIStateBase::GetCharacter() const
{
   AOSEAIController* controller = GetController();
   return controller ? controller->GetOSECharacter() : nullptr;
}

UOSEAbilitySystemComponent* UUtilityAIStateBase::GetAbilitySystemComponent() const
{
   return UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(GetCharacter());
}

FSmartObjectClaimHandle UUtilityAIStateBase::TryGetSmartObjectClaimHandleFromStateTarget() const
{
   if (_utilityAIComponent)
   {
      const FUtilityStateTarget& target = _utilityAIComponent->GetCurrentTarget();
      if (UUtilityAITokenOwnerSmartObject* smartObjTokenOwner = Cast<UUtilityAITokenOwnerSmartObject>(UUtilityAITokenOwner::AuthorityTryGetTokenOwnerFromObject(target.GetTargetUObject())))
      {
         return smartObjTokenOwner->GetClaimHandle(target);
      }
   }
   return FSmartObjectClaimHandle::InvalidHandle;
}

