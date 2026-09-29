// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "PingSystem/OSEFallbackPingTargetStrategy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEFallbackPingTargetStrategy)

// Add default functionality here for any IOSEFallbackPingTargetStrategy functions that are not pure virtual.

bool UOSEPingFallbackFunctions::FindFallbackPingTarget(const AActor* sourceActor, const FVector& startPos, const FVector& endPos, AActor*& outPingable, FHitResult& outHitResult)
{
   const IOSEFallbackPingTargetStrategy* actorTargeter = Cast<IOSEFallbackPingTargetStrategy>(sourceActor);
   if (actorTargeter && actorTargeter->FindFallbackPingTarget(startPos, endPos, outPingable, outHitResult))
   {
      return true;
   }

   for (const UActorComponent* component : sourceActor->GetComponents())
   {
      const IOSEFallbackPingTargetStrategy* targeter = Cast<IOSEFallbackPingTargetStrategy>(component);
      if (targeter && targeter->FindFallbackPingTarget(startPos, endPos, outPingable, outHitResult))
      {
         return true;
      }
   }

   return false;
}

