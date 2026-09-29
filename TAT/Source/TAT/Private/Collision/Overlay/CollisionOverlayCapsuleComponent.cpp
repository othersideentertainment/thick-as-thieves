// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Collision/Overlay/CollisionOverlayCapsuleComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(CollisionOverlayCapsuleComponent)

void UCollisionOverlayCapsuleComponent::BeginPlay()
{
   Super::BeginPlay();

   // Start applying overlays after begin play, so this only happens at runtime
   _collisionOverlay.SetBase(BodyInstance.GetResponseToChannels());
   _TryApplyOverlayResponses();
}

void UCollisionOverlayCapsuleComponent::AddCollisionOverlay(const FCollisionOverlayKey& key, const FCollisionResponseContainer& responseMask)
{
   _collisionOverlay.AddOverlay(key, responseMask);
   if (HasBegunPlay())
   {
      _TryApplyOverlayResponses();
   }
}

void UCollisionOverlayCapsuleComponent::RemoveCollisionOverlayByKey(const FCollisionOverlayKey& key)
{
   if (_collisionOverlay.RemoveOverlayByKey(key) && HasBegunPlay())
   {
      _TryApplyOverlayResponses();
   }
}

void UCollisionOverlayCapsuleComponent::SetCollisionProfileName(FName inCollisionProfileName, bool updateOverlaps)
{
   if (!HasBegunPlay())
   {
      Super::SetCollisionProfileName(inCollisionProfileName, updateOverlaps);
   }
   else
   {
      ECollisionEnabled::Type oldCollisionEnabled = BodyInstance.GetCollisionEnabled();
      BodyInstance.SetCollisionProfileName(inCollisionProfileName);
      ECollisionEnabled::Type newCollisionEnabled = BodyInstance.GetCollisionEnabled();

      // re-apply the overlay with the profile as the new base (not firing OnComponentCollisionSettingsChanged twice)
      // TODO: this will end up invalidating the BodyInstance filter multiple times, do we care? (relative to the complexity)
      _collisionOverlay.SetBase(BodyInstance.GetResponseToChannels());
      BodyInstance.SetResponseToChannels(_collisionOverlay.GetCombinedResponse());

      if (oldCollisionEnabled != newCollisionEnabled)
      {
         EnsurePhysicsStateCreated();
      }
      OnComponentCollisionSettingsChanged(updateOverlaps);
   }
}

void UCollisionOverlayCapsuleComponent::SetCollisionResponseToChannel(ECollisionChannel channel, ECollisionResponse newResponse)
{
   if (!HasBegunPlay())
   {
      Super::SetCollisionResponseToChannel(channel, newResponse);
   }
   else
   {
      _collisionOverlay.SetBaseResponseToChannel(channel, newResponse);
      _TryApplyOverlayResponses();
   }
}

void UCollisionOverlayCapsuleComponent::SetCollisionResponseToAllChannels(ECollisionResponse newResponse)
{
   if (!HasBegunPlay())
   {
      Super::SetCollisionResponseToAllChannels(newResponse);
   }
   else
   {
      _collisionOverlay.SetBase(FCollisionResponseContainer(newResponse));
      _TryApplyOverlayResponses();
   }
}

void UCollisionOverlayCapsuleComponent::SetCollisionResponseToChannels(const FCollisionResponseContainer& newResponses)
{
   if (!HasBegunPlay())
   {
      Super::SetCollisionResponseToChannels(newResponses);
   }
   else
   {
      _collisionOverlay.SetBase(newResponses);
      _TryApplyOverlayResponses();
   }
}

void UCollisionOverlayCapsuleComponent::_TryApplyOverlayResponses()
{
   if(BodyInstance.SetResponseToChannels(_collisionOverlay.GetCombinedResponse()))
   {
      OnComponentCollisionSettingsChanged();
   }
}

