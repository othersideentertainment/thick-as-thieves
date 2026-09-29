// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Reactions/TATAIReactionRole.h"

// tat
#include "AI/Reactions/TATAIReactionEvent.h"
#include "AI/Reactions/TATAIReactionSettings.h"
#include "Character/TATCharacterAIBase.h"

// ue
#include "EnvironmentQuery/EnvQuery.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIReactionRole)

ETATAIReactionRegistrationState FTATAIReactionRole::TryRegisterAI(FTATAIReactionRegistrationContext& context, const FTATAIReactionEvent& owningEvent)
{
   if (IsDefaultRole())
   {
      // The default role doesn't have any criteria to meet.
      // Any and all AI can join the default role.
      _roster.Registrants.Add(context.In.AICharacter);
      return ETATAIReactionRegistrationState::Registrant;
   }

   const FTATAIConditionalReactionRoleConfig* config = GetConfig();
   check(config);

   if (_roster.Registrants.Num() < config->MaxRegistrants || config->MaxRegistrants == 0)
   {
      // If we're not yet at max, registered the AI for the role.
      _roster.Registrants.Add(context.In.AICharacter);
      return ETATAIReactionRegistrationState::Registrant;
   }
   
   const float eventStartTime = owningEvent.StartTime;
   const float eventGracePeriod = config->RegistrationGracePeriod;
   const float currentTime = context.In.CurrentTime;

   if (currentTime < eventStartTime + eventGracePeriod)
   {
      // If we are within the registration grace period, mark this potential registrant as
      // overflow, which indicates that we should eventually consider them with the set of AI
      // already registered to determine who is "best fit" for this role.
      _roster.Overflow.Add(context.In.AICharacter);
      return ETATAIReactionRegistrationState::Overflow;
   }
   else
   {
      // If we've missed the registration grace period, we're out of luck.
      return ETATAIReactionRegistrationState::None;
   }
}

ETATAIReactionRegistrationState FTATAIReactionRole::TryUnregisterAI(FTATAIReactionRegistrationContext& context)
{
   if (_roster.Registrants.Remove(context.In.AICharacter) > 0)
   {
      return ETATAIReactionRegistrationState::Registrant;
   }
   else if (_roster.Overflow.Remove(context.In.AICharacter) > 0)
   {
      return ETATAIReactionRegistrationState::Overflow;
   }
   else
   {
      return ETATAIReactionRegistrationState::None;
   }
}

int32 FTATAIReactionRole::GetNumRegistrantOpenings() const
{
   if (IsDefaultRole())
   {
      // Default roles have no limit on the number of registrants.
      return INT_MAX;
   }
   else
   {
      const FTATAIConditionalReactionRoleConfig* config = GetConfig();
      if(config)
      {
         return (config->MaxRegistrants - _roster.Registrants.Num());
      }
      else
      {
         // An error within GetConfig will fire letting us know which event is having this issue, instead of crashing with no logging, return zero
         // so that the role is considered "occupied".
         return 0;
      }
   }
}

const FTATAIConditionalReactionRoleConfig* FTATAIReactionRole::GetConfig() const
{
   return UTATAIReactionSettings::Get().GetConditionalRoleConfig(_owningEventConfigId, ConfigId);
}

void FTATAIReactionRole::ApplyEvaluatorResults(FTATAIReactionRoleEvaluatorResults& results)
{
   // EQS evaluators should never run on default roles (so this method should
   // never be called on one).
   check(ConfigId.IsValid());

   const FTATAIConditionalReactionRoleConfig* config = GetConfig();
   check(config);

   const int maxRegistrants = config->MaxRegistrants;
   const int numAIEvaluated = results.In.QueryScoreSortedAI.Num();

   // The query can happen over multiple frames, in that time an NPC might have unregistered from the roster an effectively invalidated this query.
   // So to be safe, lets make sure we're pulling the lowest number between the max registrants and the number of AI evaluated
   const int safeRegistrantsBelowOverflow = FMath::Min(maxRegistrants, numAIEvaluated);
   
   // First, make sure we set the registrants to the highest scorers from the
   // evaluator.
   int i = 0;
   for (; i < safeRegistrantsBelowOverflow; i++)
   {
      // EQS evaluator queries should only ever be operating on AI characters
      const ATATCharacterAIBase* aiCharacter = CastChecked<ATATCharacterAIBase>(results.In.QueryScoreSortedAI[i]);

      // The lowest index AI in QueryScoreSortedAI are those deemed "best fit" for this role.
      // If the AI was previously in Overflow, remove them and add them to Registrants.
      // If the AI was previously in Registrants, no need to do anything - we can simply leave them there.
      // If the AI cannot be found in either, they may have been unregistered since this EQS evaluator was started,
      // in which case there is no need to do anything.
      const bool wasAIOverflow = (_roster.Overflow.Remove(aiCharacter) > 0);
      if (wasAIOverflow)
      {
         _roster.Registrants.Add(aiCharacter);

         const FSetElementId setId = results.Out.NewRegistrants.Emplace(aiCharacter);
         FTATAIRegisteredRoleChangeData& changeData = results.Out.NewRegistrants.Get(setId);
         changeData.NewRole = results.RoleId.RoleTag;
      }
   }

   // Second, for any AI that didn't make it into the registrants "best fit" set, make sure
   // we remove them from overflow (as they were evaluated). 
   for (; i < numAIEvaluated; i++)
   {
      // EQS evaluator queries should only ever be operating on AI characters
      const ATATCharacterAIBase* aiCharacter = CastChecked<ATATCharacterAIBase>(results.In.QueryScoreSortedAI[i]);

      const bool wasAIRegistrant = (_roster.Registrants.Remove(aiCharacter) > 0);
      _roster.Overflow.Remove(aiCharacter);
      if (wasAIRegistrant)
      {
         const FSetElementId setId = results.Out.Orphans.Emplace(aiCharacter);
         FTATAIRegisteredRoleChangeData& changeData = results.Out.Orphans.Get(setId);
         changeData.OldRole = results.RoleId.RoleTag;
      }
   }

   // After the second step (loop), we should definitely not be over the max number of
   // registrants for this role. Most often, Registrants will be at max, but it is 
   // possible that an AI is unregistered after the EQS evaluator was started and thus
   // there are less now.
   check(_roster.Registrants.Num() <= maxRegistrants);
}
