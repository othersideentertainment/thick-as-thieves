// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/Effects/TATTokenClueEffect.h"

// tat
#include "Items/Tokens/TATTokenInventoryComponent.h"

// ue
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTokenClueEffect)

void FTATTokenClueEffect::ApplyTo(APlayerState* playerState) const
{
   if(UTATTokenInventoryComponent* inventory = UTATTokenInventoryComponent::FromActor(playerState))
   {
      for(TObjectPtr<const UTATInventoryToken> token : TokensToGrant)
      {
         if(token == nullptr)
         {
            continue;
         }

         if(AllowDuplicates || !inventory->HasToken(token))
         {
            inventory->AuthorityAddToken(token);
         }
      }
   }
}

#if WITH_EDITOR
void FTATTokenClueEffect::Validate(TFunctionRef<void(const FText&)> reportError) const
{
    // TODO
}
#endif
