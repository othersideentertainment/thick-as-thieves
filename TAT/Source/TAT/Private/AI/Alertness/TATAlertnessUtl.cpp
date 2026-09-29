// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Alertness/TATAlertnessUtl.h"

// tat
#include "GameplayTagAssetInterface.h"
#include "AI/TATAIFunctionLibrary.h"
#include "AI/TATAISettings.h"
#include "AI/TATKnowledgeComponent.h"
#include "AI/Alertness/TATAlertnessGameplayTags.h"
#include "Developer/TATProjectSettings.h"
#include "Environment/TATPrivateSpaceGameplayTagDefines.h"

bool AlertnessUtl::CanCharacterDecayAlertnessToNeutral(AActor* actor, EAlertnessLevel currentAlertnessLevel)
{
   ensure(actor);

   bool canDecay = true;

   const IGameplayTagAssetInterface* tagAssetInterface = Cast<IGameplayTagAssetInterface>(actor);
   if(tagAssetInterface && tagAssetInterface->HasMatchingGameplayTag(TAG_STATUS_ALERTNESS_BLOCKTRANSITIONDECAY))
   {
      return false;
   }
   
   switch(currentAlertnessLevel)
   {
   case EAlertnessLevel::Neutral:
      {
         // no decay from neutral
         canDecay = false;
      }
      break;
   case EAlertnessLevel::Suspicious:
      {
         const UTATProjectSettings& settings = UTATProjectSettings::Get();
               
         FGameplayTagContainer tagsToCheck;
         tagsToCheck.AddTag(settings.SuspiciousActionStatusTag);
         tagsToCheck.AddTag(TAG_STATUS_PRIVATESPACE_INTRUDING);
               
         // we decay from suspicious based on our detection levels. 
         UTATAIFunctionLibrary::AuthorityForEachPlayerKnowledge(actor,
            [&canDecay, &tagsToCheck](const FTATActorKnowledge& actorKnowledge, const APawn& pawn)
            {
               const IGameplayTagAssetInterface* tagAssetInterface = Cast<IGameplayTagAssetInterface>(actorKnowledge.GetActor());

               if(tagAssetInterface && tagAssetInterface->HasAnyMatchingGameplayTags(tagsToCheck))
               {
                  if(actorKnowledge.GetDetectionValue() > 0.0f)
                     canDecay = false;
               }
            }
         );
      }
      break;
   case EAlertnessLevel::Combat:
   case EAlertnessLevel::Alerted:
      {
         // we decay from alert/combat by ourselves based on our detection levels
         UTATAIFunctionLibrary::AuthorityForEachEnemyKnowledge(actor,
            [&canDecay](const FTATActorKnowledge& actorKnowledge)
            {
               if (actorKnowledge.GetDetectionState() == EActorDetectionState::Identified || 
                     actorKnowledge.GetDetectionValue() > 0.0f)
               {
                  canDecay = false;
               }
            }
         );
      }
      break;
   case EAlertnessLevel::MAX:
      checkNoEntry();
      break;
   }
   
   return canDecay;
}
