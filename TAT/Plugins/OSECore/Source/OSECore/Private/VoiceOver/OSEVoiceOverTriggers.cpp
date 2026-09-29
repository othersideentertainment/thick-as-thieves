// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverTriggers.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Online/OSEGameState.h"

// ue5
#include "GameplayEffect.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverTriggers)


void UOSEVoiceOverTriggers::FireDamageVO(AActor* actor, const FGameplayEffectSpec& spec)
{
   if (actor->HasAuthority())
   {
      FGameplayTagContainer tags;
      spec.GetAllAssetTags(tags);

      for (const FOSEDamageVoiceOverTrigger& trigger : DamageTriggers)
      {
         if (trigger.VoiceOverParams.Line)
         {
            if (tags.HasTag(trigger.Tag))
            {
               trigger.VoiceOverParams.AuthoritySubmitRequest(actor);
            }
         }
      }
   }
}

