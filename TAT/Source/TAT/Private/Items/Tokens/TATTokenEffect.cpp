// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/Tokens/TATTokenEffect.h"

// tat
#include "Environment/TATPrivateSpaceCharacterComponent.h"
#include "Environment/TATPrivateSpaceCharacterInterface.h"

// ose
#include "OSECommon.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTokenEffect)

namespace TokenEffectHelpers
{
   static UTATPrivateSpaceCharacterComponent* GetPrivateSpaceComponent(AActor* actor)
   {
      if(auto* privateSpaceInterface = Cast<ITATPrivateSpaceCharacterInterface>(UOSECommon::GetPawn(actor)))
      {
         return privateSpaceInterface->GetPrivateSpaceCharacterComponent();
      }

      return nullptr;
   }
}

void FTATTokenEffect_PrivateSpace::OnAdded(AActor* holdingActor) const
{
   if(UTATPrivateSpaceCharacterComponent* privateSpaceComponent = TokenEffectHelpers::GetPrivateSpaceComponent(holdingActor))
   {
      for(const FGameplayTag& tag : SpaceTags)
      {
         privateSpaceComponent->AuthorityAddTemporaryAllowedPrivateZone(tag);
      }
   }
}

void FTATTokenEffect_PrivateSpace::OnRemoved(AActor* holdingActor) const
{
   if(UTATPrivateSpaceCharacterComponent* privateSpaceComponent = TokenEffectHelpers::GetPrivateSpaceComponent(holdingActor))
   {
      for(const FGameplayTag& tag : SpaceTags)
      {
         privateSpaceComponent->AuthorityRemoveTemporaryAllowedPrivateZone(tag);
      }
   }
}
