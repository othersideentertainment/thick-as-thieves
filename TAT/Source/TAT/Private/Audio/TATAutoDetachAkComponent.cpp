// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Audio/TATAutoDetachAkComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAutoDetachAkComponent)


UTATAutoDetachAkComponent::UTATAutoDetachAkComponent()
   : Super(FObjectInitializer::Get())
{

}


void UTATAutoDetachAkComponent::BeginPlay()
{
   Super::BeginPlay();

   // If dedicated server, the tick logic will not run, so detach unconditionally
   // without keeping track of previous attachment
   if (_autoDetachWhenNotPlaying && IsNetMode(NM_DedicatedServer))
   {
      DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
   }
}

void UTATAutoDetachAkComponent::_UpdateAutoDetach()
{
   // Would be nice not to poll, but that would require modding wwise
   const bool isPlaying = HasActiveEvents();
   const bool currentlyAttached = GetAttachParent() != nullptr;

   if(currentlyAttached == isPlaying)
   {
      return;
   }
   
   if(currentlyAttached)
   {
      _previousAttachComponent = GetAttachParent();
      _previousSocket = GetAttachSocketName();
      DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);
   }
   else
   {
      if(USceneComponent* parentToRestore = _previousAttachComponent.Get())
      {
         AttachToComponent(parentToRestore, FAttachmentTransformRules::KeepRelativeTransform, _previousSocket);
      }
   }
}

void UTATAutoDetachAkComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   if(_autoDetachWhenNotPlaying)
   {
      _UpdateAutoDetach();
   }
   
   Super::TickComponent(deltaTime, tickType, thisTickFunction);
}

