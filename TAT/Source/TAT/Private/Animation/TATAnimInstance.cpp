// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/TATAnimInstance.h"
#include "Animation/TATAnimInstanceProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAnimInstance)


//--------------------------------------------------------------------------------------------------
// UTATAnimInstance
//--------------------------------------------------------------------------------------------------

UTATAnimInstance::UTATAnimInstance()
   : UOSEAnimInstance()
{ }

FAnimInstanceProxy* UTATAnimInstance::CreateAnimInstanceProxy()
{
   return new FTATAnimInstanceProxy(this);
}

bool UTATAnimInstance::HandleNotify(const FAnimNotifyEvent& animNotifyEvent)
{
   // Returning true in this method indicates that we "handled" the anim notify ourselves, suppressing the default behavior
   if (!_enableAnimNotifies)
   {
      return true;
   }
   return Super::HandleNotify(animNotifyEvent);
}

bool UTATAnimInstance::ShouldTriggerAnimNotifyState(const UAnimNotifyState* animNotifyState) const
{
   if (!_enableAnimNotifies)
   {
      return false;
   }
   return Super::ShouldTriggerAnimNotifyState(animNotifyState);
}

void UTATAnimInstance::PerformInitialize()
{
   Super::PerformInitialize();

   // Disable overlap updates for perf
   USkeletalMeshComponent* skeletalMeshComponent = GetOwningComponent();
   skeletalMeshComponent->bUpdateOverlapsOnAnimationFinalize = false;

   // Init ability data
   CurrentAbilityData = FTATAnimAbilityData();
   PreviousAbilityData = FTATAnimAbilityData();
   AIData = FTATAIAnimData();
}

void UTATAnimInstance::PerformDataCopy(const UOSEAnimInstance* srcInstance)
{
   Super::PerformDataCopy(srcInstance);

   if (const UTATAnimInstance* tatInstance = Cast<UTATAnimInstance>(srcInstance))
   {
      // Copy ability data
      CurrentAbilityData = tatInstance->CurrentAbilityData;
      PreviousAbilityData = tatInstance->PreviousAbilityData;
      AIData = tatInstance->AIData;
      CachedPlayingAnyMontage = tatInstance->CachedPlayingAnyMontage;
   }
}

void UTATAnimInstance::PerformDataUpdate()
{
   Super::PerformDataUpdate();

   // Save previous ability data and update current ability data
   PreviousAbilityData = CurrentAbilityData;
   CurrentAbilityData.Update(GetActorInfo());
   AIData.Update(GetActorInfo());
   CachedPlayingAnyMontage = IsAnyMontagePlaying();
}

