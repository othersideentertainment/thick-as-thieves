// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Reactions/TATAIReactionEvent.h"

// tat
#include "AI/Reactions/TATAIReactionSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIReactionEvent)
DEFINE_LOG_CATEGORY_STATIC(LogTATAIReactionEvent, Log, All);

bool FTATAIReactionEventConfig::ContainsRole(const FGameplayTag& roleTag) const
{
   if (roleTag.MatchesTagExact(DefaultRole))
   {
      return true;
   }

   for (const FTATAIConditionalReactionRoleConfig& roleConfig : ConditionalRoles)
   {
      if (roleTag.MatchesTagExact(roleConfig.RoleTag))
      {
         return true;
      }
   }

   return false;
}

void FTATAIReactionEvent::RegisterAI(FTATAIReactionRegistrationContext& context)
{
   if (!context.In.IsValid())
   {
      UE_LOG(LogTATAIReactionEvent, Error, TEXT("FTATAIReactionEvent::RegisterAI passed an invalid context!"));
      context.Out.State = ETATAIReactionRegistrationState::None;
      return;
   }

   // The priority order of conditional roles are stored in the config.
   const FTATAIReactionEventConfig* config = GetConfig();
   if (config == nullptr)
   {
      UE_LOG(LogTATAIReactionEvent, Error, 
         TEXT("RegisterAI failed to retrieve the associated reaction event config! Failed to register %s for a role!"),
         *GetNameSafe(context.In.AICharacter));
      context.Out.State = ETATAIReactionRegistrationState::None;
      return;
   }

   auto findOrAddRole = [&](const FGameplayTag& roleTag, const FTATAIConditionalReactionRoleConfigId& roleId) -> FTATAIReactionRole*
   {
      if (FTATAIReactionRole* role = _roles.Find(roleTag))
      {
         return role;
      }
      else
      {
         return &_roles.Emplace(roleTag, FTATAIReactionRole(ConfigId, roleId));
      }
   };

   ETATAIReactionRegistrationState roleRegistrationState = ETATAIReactionRegistrationState::None;;
   for (int i = context.In.StartingRoleId.RoleIndex; i < config->ConditionalRoles.Num(); i++)
   {
      const FTATAIConditionalReactionRoleConfig& roleConfig = config->ConditionalRoles[i];
      FTATAIReactionRole* role = findOrAddRole(roleConfig.RoleTag, FTATAIConditionalReactionRoleConfigId(i));
      roleRegistrationState = role->TryRegisterAI(context, *this);

      switch (roleRegistrationState)
      {
         // If the AI was successfully registered, we're done.
         case ETATAIReactionRegistrationState::Registrant:
            context.Out.RoleTag = roleConfig.RoleTag;
            _numConditionalRoleRegistrants++;
#if DO_ENSURE
            ensure(_numConditionalRoleRegistrants == _CalcNumConditionalRoleRegistrants());
#endif
            return;

         case ETATAIReactionRegistrationState::Overflow:
            context.Out.State = ETATAIReactionRegistrationState::Overflow;
            break;
      }
   }

   if (config->DefaultRole.IsValid())
   {
      FTATAIConditionalReactionRoleConfigId dummyId;
      FTATAIReactionRole* defaultRole = findOrAddRole(config->DefaultRole, dummyId);
      roleRegistrationState = defaultRole->TryRegisterAI(context, *this);
      context.Out.RoleTag = config->DefaultRole;
      checkf(roleRegistrationState == ETATAIReactionRegistrationState::Registrant,
         TEXT("Failed to register %s for a DefaultRole (%s). AI should always be able to register with a DefaultRole."),
         *GetNameSafe(context.In.AICharacter), *config->DefaultRole.ToString());
   }
}

void FTATAIReactionEvent::UnregisterAI(FTATAIReactionRegistrationContext& context)
{
   context.Out.State = ETATAIReactionRegistrationState::None;

   if (!context.In.IsValid())
   {
      UE_LOG(LogTATAIReactionEvent, Error, TEXT("FTATAIReactionEvent::UnregisterAI passed an invalid context!"));
      return;
   }

   // The priority order of conditional roles are stored in the config.
   const FTATAIReactionEventConfig* config = GetConfig();
   if (config == nullptr)
   {
      UE_LOG(LogTATAIReactionEvent, Error,
         TEXT("UnregisterAI failed to retrieve the associated reaction event config! Failed to unregister %s for a role!"),
         *GetNameSafe(context.In.AICharacter));
      return;
   }

   ETATAIReactionRegistrationState roleUnregistrationState = ETATAIReactionRegistrationState::None;
   for (int i = context.In.StartingRoleId.RoleIndex; i < config->ConditionalRoles.Num(); i++)
   {
      const FTATAIConditionalReactionRoleConfig& roleConfig = config->ConditionalRoles[i];
      if (FTATAIReactionRole* role = _roles.Find(roleConfig.RoleTag))
      {
         roleUnregistrationState = role->TryUnregisterAI(context);

         switch (roleUnregistrationState)
         {
            // If we found the role this AI was registered with, they shouldn't have
            // been overflowed with any further (lower priority) roles. We're done.
            case ETATAIReactionRegistrationState::Registrant:
               context.Out.RoleTag = roleConfig.RoleTag;
               context.Out.State = ETATAIReactionRegistrationState::Registrant;
               _numConditionalRoleRegistrants--;
#if DO_ENSURE
               ensure(_numConditionalRoleRegistrants == _CalcNumConditionalRoleRegistrants());
#endif
               return;

            case ETATAIReactionRegistrationState::Overflow:
               context.Out.State = ETATAIReactionRegistrationState::Overflow;
               break;
         }
      }
      else
      {
         // no one has registered for these, no need to continue
         break;
      }
   }

   if (FTATAIReactionRole* defaultRole = GetRole(config->DefaultRole))
   {
      roleUnregistrationState = defaultRole->TryUnregisterAI(context);
      context.Out.State = roleUnregistrationState;
      if (context.Out.State == ETATAIReactionRegistrationState::Registrant)
      {
         context.Out.RoleTag = config->DefaultRole;
      }
   }
}

bool FTATAIReactionEvent::IsFinished() const
{
   if (HasEvaluatorInFlight())
   {
      // If an evaluator is currently being run for this event, at least
      // allow it to finish before ending the event.
      return false;
   }

   const FTATAIReactionEventConfig* config = GetConfig();
   if (config == nullptr)
   {
      // If we can't find the config, something is wrong with this event.
      // Consider it finished.
      return true;
   }

   // If we have any conditional role registrants, this event is still ongoing.
   if (_numConditionalRoleRegistrants > 0)
   {
      return false;
   }

   // If we've gotten here then no conditional roles were found.
   if (config->EndEventWhenNoConditionalRolesAssigned)
   {
      return true;
   }
   else
   {
      // If 'defaultRole' is null, that means no one has registered with it.
      const FTATAIReactionRole* defaultRole = GetRole(config->DefaultRole);
      return (defaultRole == nullptr) || (defaultRole->GetRoster().Registrants.Num() == 0);
   }
}

FTATAIReactionRole* FTATAIReactionEvent::GetRole(const FGameplayTag& roleTag)
{
   return _roles.Find(roleTag);
}

const FTATAIReactionRole* FTATAIReactionEvent::GetRole(const FGameplayTag& roleTag) const
{
   return _roles.Find(roleTag);
}

const FTATAIReactionEventConfig* FTATAIReactionEvent::GetConfig() const
{
   return UTATAIReactionSettings::Get().GetEventConfig(ConfigId);
}

void FTATAIReactionEvent::ApplyEvaluatorResults(FTATAIReactionRoleEvaluatorResults& results)
{
   // Ensure evaluators are only applied to the event they were run for.
   check(Target == results.RoleId.Target);

   // If an evaluator was run on a role, then the role should exist
   // (roles exist for the lifetime of the event)
   FTATAIReactionRole* role = GetRole(results.RoleId.RoleTag);
   check(role);

   // Evaluators should never be run on default roles because they have no limit
   // to registrants. Therefore, all evaluators should run on conditional roles.

   // While applying evaluator results should often result in the same number of
   // registrants, there is always the chance that a registrants was removed while
   // the evaluator was running. To cover our bases, remove the registrant count
   // and re-add it when the results have been applied.
   _numConditionalRoleRegistrants -= role->GetRoster().Registrants.Num();

   role->ApplyEvaluatorResults(results);

   _numConditionalRoleRegistrants += role->GetRoster().Registrants.Num();
#if DO_ENSURE
   ensure(_numConditionalRoleRegistrants == _CalcNumConditionalRoleRegistrants());
#endif

   // Evaluators should never be run on default roles, so there should always be a next role.
   FTATAIConditionalReactionRoleConfigId nextRole = role->ConfigId.GetNext();
   check(nextRole.IsValid());

   // Try to unregister the AI that were previoiusly registered with lower priority roles.
   for (FTATAIRegisteredRoleChangeData& newRegistrant : results.Out.NewRegistrants)
   {
      FTATAIReactionRegistrationContext context(Target, newRegistrant.AICharacter.Get(), results.In.CurrentTime, nextRole);
      UnregisterAI(context);

      // Cache where the AI was previously registered.
      // Note: it is possible that an AI may have been registered to nothing previously
      // such as if the reaction event only has one conditional role and no default role.
      newRegistrant.OldRole = context.Out.RoleTag;
   }

   // Try to find new roles for the orphans.
   for (FTATAIRegisteredRoleChangeData& orphan : results.Out.Orphans)
   {
      FTATAIReactionRegistrationContext context(Target, orphan.AICharacter.Get(), results.In.CurrentTime, nextRole);
      RegisterAI(context);

      // Cache where the AI is now registered.
      // Note: it is possible that an AI may not be able to register to anything
      // such as if the reaction event only has one conditional role and no default role.
      orphan.NewRole = context.Out.RoleTag;
   }
}

#if DO_ENSURE
int32 FTATAIReactionEvent::_CalcNumConditionalRoleRegistrants() const
{
   int32 numRegistrants = 0;
   if (const FTATAIReactionEventConfig* config = GetConfig())
   {
      for (int i = 0; i < config->ConditionalRoles.Num(); i++)
      {
         const FTATAIConditionalReactionRoleConfig& roleConfig = config->ConditionalRoles[i];
         if (const FTATAIReactionRole* role = GetRole(roleConfig.RoleTag))
         {
            numRegistrants += role->GetRoster().Registrants.Num();
         }
         else
         {
            // If no one has attempted to register with this role (and thus
            // implying lower priority roles as well), and we haven't found any
            // registrants before this, then there will be none any further.
            break;
         }
      }
   }
   return numRegistrants;
}
#endif
