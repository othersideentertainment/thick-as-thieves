// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/TATQuestDependentActorHelpers.h"

// tat
#include "Player/TATLocalPlayerStateWorldSubsystem.h"
#include "Player/TATPlayerState.h"
#include "Quests/TATActiveQuestSubsystem.h"
#include "Quests/TATQuestTags.h"

// ose
#include "OSECommon.h"

// ue
#include "GameFramework/Character.h"


bool TATQuestDependentActorHelpers::IsContract(FGameplayTag possibleQuestTag)
{
   return possibleQuestTag.MatchesTag(TAG_Contract);
}

bool TATQuestDependentActorHelpers::IsRelevantForCharacter(FGameplayTag possibleQuestTag, const ACharacter* character)
{
   if(!IsContract(possibleQuestTag))
   {
      return true;
   }

   check(character);
   const ATATPlayerState* playerState = character->GetPlayerState<ATATPlayerState>();
   return playerState && playerState->GetActiveQuestTags().Contains(possibleQuestTag);
}

bool TATQuestDependentActorHelpers::AuthorityIsRelevantToViewingActor(FGameplayTag possibleQuestTag, const AActor* viewingActor)
{
   if(!IsContract(possibleQuestTag))
   {
      return true;
   }

   check(viewingActor && viewingActor->HasAuthority());
   const ATATPlayerState* playerState = UOSECommon::GetPlayerState<const ATATPlayerState>(viewingActor);
   return playerState && playerState->GetActiveQuestTags().Contains(possibleQuestTag);
}

bool TATQuestDependentActorHelpers::IsRelevantToListenServerPlayer(FGameplayTag possibleQuestTag, const AActor* contextActor)
{
   if(!IsContract(possibleQuestTag))
   {
      return true;
   }
   
   check(contextActor && contextActor->IsNetMode(NM_ListenServer));

   constexpr bool useFixedPlayerId = true;
   if constexpr (useFixedPlayerId)
   {
      const UTATActiveQuestSubsystem* activeQuestSubsystem = contextActor->GetWorld()->GetSubsystem<UTATActiveQuestSubsystem>();
      return activeQuestSubsystem && activeQuestSubsystem->GetContract() == possibleQuestTag;
   }
   else
   {
      const ATATPlayerState* playerState = ATATPlayerState::GetLocalTATPlayerState(contextActor);
      return playerState && playerState->GetActiveQuestTags().Contains(possibleQuestTag);
   }
}
