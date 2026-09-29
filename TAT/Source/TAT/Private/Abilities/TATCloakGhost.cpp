// (c) 2021-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATCloakGhost.h"

// OSE
#include "AI/OSEAIFunctionLibrary.h"
#include "AI/Utility/UtilityAITokenOwnerGameplayTagCount.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCloakGhost)

ATATCloakGhost::ATATCloakGhost()
   :Super()
{
}

void ATATCloakGhost::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      const TArray<FOSEAITokenInfo> defaultTokens = { DefaultAITokens };
      const TArray<FOSEAITokenInfo> maxTokenDebt;
      _tokenOwner = UUtilityAITokenOwnerGameplayTagCount::AuthorityCreate(this, defaultTokens, maxTokenDebt);
   }
}

bool ATATCloakGhost::CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor, const bool* wasVisible, int32* userData) const
{
   FHitResult hitResult;
   const bool canBeSeen = UOSEAIFunctionLibrary::SightSenseLineTrace(hitResult, observerLocation, this, ignoreActor);
   numberOfLoSChecksPerformed = 1;
   if (canBeSeen)
   {
      outSeenLocation = GetActorLocation();
      outSightStrength = 1.0f; // TODO: We aren't using this for anything, so...?
      return true;
   }

   outSightStrength = 0;
   return false; 
}

void ATATCloakGhost::AuthorityOnEnterVisibleByActor(AActor* viewingActor)
{
   BP_AuthorityOnEnterVisibleByActor(viewingActor);
}

void ATATCloakGhost::AuthorityOnExitVisibleByActor(AActor* viewingActor)
{
   BP_AuthorityOnExitVisibleByActor(viewingActor);
}

