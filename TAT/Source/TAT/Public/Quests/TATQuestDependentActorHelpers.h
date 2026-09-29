// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Player/TATLocalPlayerStateWorldSubsystem.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

// Some utilities for actors that may be contract-specific
//
// Initially for certain clues
//
// NOTE: All current functions assume that any mission tags are relevant. If this
//       is used in cases where a mission tag is provided that is not the active
//       mission, these should also be changed to handle that (but that is easy).
// NOTE: With the new model of players being able to participate in contracts of allies, this
//       may no longer be necessary. Not removing just yet.
namespace TATQuestDependentActorHelpers
{
   bool IsContract(FGameplayTag possibleQuestTag);

   // For use in IsInteractable polling, or similar
   bool IsRelevantForCharacter(FGameplayTag possibleQuestTag, const ACharacter* character);

   // For things that exist in the level, and would not disappear, bind to
   // OnceRelevantToLocalPlayer or UTATLocalPlayerStateSubsystem::OnLocalQuestChanged, to make cosmetic changes
   // and check IsRelevantForCharacter for interaction
   
   // A convenience function to bind to for cosmetic changes (about say clue available ability)
   // Not explicitly de-duped, so should not be used where it would be called a lot for a given thing, or
   // if there are multiple clues in an actor
   //
   // Raw capture of this pointer (where that is the target) is okay, but not any other raw pointers
   template <typename TUserObject, typename THandler>
   void OnceRelevantToLocalPlayer(FGameplayTag possibleQuestTag, const TUserObject* target, THandler&& handler)
   {
      const UWorld* world = target->GetWorld();
      check(world && !world->IsNetMode(NM_DedicatedServer));

      if (!IsContract(possibleQuestTag))
      {
         handler();
         return;
      }

      auto* localPlayerStateSubsystem = world->GetSubsystem<UTATLocalPlayerStateWorldSubsystem>();
      check(localPlayerStateSubsystem);

      FGameplayTag currentQuest = localPlayerStateSubsystem->GetLocalContract();
      if (currentQuest == possibleQuestTag)
      {
         handler();
         return;
      }

      if (currentQuest.IsValid())
      {
         // known to be a different quest, so don't subscribe
         return;
      }

      localPlayerStateSubsystem->OnLocalContractSet.AddWeakLambda(target, [tag = possibleQuestTag, handler = MoveTemp(handler)](FGameplayTag contract)
         {
            if (contract == tag)
            {
               handler();
            }
         });
   }
   
   // For use in IsNetRelevant for actors that are positively spawned, and are not in the level
   bool AuthorityIsRelevantToViewingActor(FGameplayTag possibleQuestTag, const AActor* viewingActor);
   // For use by listen servers that primarily use relevancy, but need to make sure something is off enough on the listen server
   // The listen server should be able to know without binding to the localPlayerState subsystem
   bool IsRelevantToListenServerPlayer(FGameplayTag possibleQuestTag, const AActor* contextActor);
}
