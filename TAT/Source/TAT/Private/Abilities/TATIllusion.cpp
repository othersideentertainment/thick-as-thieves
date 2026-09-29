// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATIllusion.h"

// ose
#include "AI/OSEAIFunctionLibrary.h"
#include "AI/Utility/UtilityAITokenOwnerGameplayTagCount.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATIllusion)

void ATATIllusion::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      const TArray<FOSEAITokenInfo> defaultTokens = { DefaultAITokens };
      const TArray<FOSEAITokenInfo> maxTokenDebt;
      _tokenOwner = UUtilityAITokenOwnerGameplayTagCount::AuthorityCreate(this, defaultTokens, maxTokenDebt);
   }
}

bool ATATIllusion::CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor, const bool* wasVisible, int32* userData) const
{
   FHitResult hitResult;
   const bool canBeSeen = UOSEAIFunctionLibrary::SightSenseLineTrace(hitResult, observerLocation, this, ignoreActor);
   numberOfLoSChecksPerformed = 1;
   if (canBeSeen)
   {
      outSeenLocation = GetActorLocation();
      outSightStrength = SightStrengthMultiplier;
      return true;
   }

   outSightStrength = 0;
   return false; 
}

