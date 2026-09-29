// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverLineRequestParams.h"

// ose
#include "VoiceOver/OSEVoiceOverControllerComponent.h"
#include "Online/OSEGameState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverLineRequestParams)



FOSEVoiceOverLineRequestParams::FOSEVoiceOverLineRequestParams(UOSEVoiceOverLine* line, FGameplayTag priorityTag, bool interrupting, float timeInQueue)
   : Line(line)
   , PriorityTag(priorityTag)
   , Interrupting(interrupting)
   , TimeInQueue(timeInQueue)
{
}

void FOSEVoiceOverLineRequestParams::AuthoritySubmitRequest(AActor* voiceActor) const
{
   if (voiceActor && voiceActor->HasAuthority() && Line != nullptr)
   {
      UWorld* world = voiceActor->GetWorld();
      check(world);

      AOSEGameState* gameState = Cast<AOSEGameState>(world->GetGameState());
      check(gameState);

      if (UOSEVoiceOverControllerComponent* voController = gameState->GetVOController())
      {
         if (Line->HasAnyLines(voiceActor))
         {
            voController->AuthorityRequestVOLine(voiceActor, Line, PriorityTag, Interrupting, TimeInQueue);
         }
      }
   }
}
