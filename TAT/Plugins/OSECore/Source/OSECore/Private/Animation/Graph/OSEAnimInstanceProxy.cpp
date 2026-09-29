// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/Graph/OSEAnimInstanceProxy.h"
#include "Animation/Graph/OSEAnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAnimInstanceProxy)


//--------------------------------------------------------------------------------------------------
// FOSEAnimInstanceProxy
//--------------------------------------------------------------------------------------------------

FOSEAnimInstanceProxy::FOSEAnimInstanceProxy()
   : FAnimInstanceProxy()
{

}

FOSEAnimInstanceProxy::FOSEAnimInstanceProxy(UAnimInstance* inAnimInstance)
   : FAnimInstanceProxy(inAnimInstance)
{

}

void FOSEAnimInstanceProxy::PreUpdate(UAnimInstance* inAnimInstance, float deltaSeconds)
{
   Super::PreUpdate(inAnimInstance, deltaSeconds);

   if (auto animInst = Cast<UOSEAnimInstance>(inAnimInstance))
   {
      // Copy the actor info from the instance
      _actorInfo = animInst->GetActorInfo();
   }
}

void FOSEAnimInstanceProxy::Update(float deltaSeconds)
{
   Super::Update(deltaSeconds);
}

void FOSEAnimInstanceProxy::PostUpdate(UAnimInstance* inAnimInstance) const
{
   Super::PostUpdate(inAnimInstance);
}

