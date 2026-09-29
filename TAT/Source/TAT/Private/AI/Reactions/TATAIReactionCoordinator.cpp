// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Reactions/TATAIReactionCoordinator.h"

// tat
#include "AI/TATAIController.h"
#include "AI/TATAISettings.h"
#include "AI/TATKnowledgeComponent.h"
#include "AI/Reactions/TATAIReactionSettings.h"
#include "AI/StateTrees/TATStateTreeEvents.h"
#include "Character/TATCharacterAIBase.h"

// ue
#include "Components/StateTreeComponent.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "GameFramework/Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIReactionCoordinator)
DEFINE_LOG_CATEGORY_STATIC(LogTATAIReactionCoordinator, Log, All);

namespace TATAIReactionCoordinatorHelpers
{
   static UStateTreeComponent* GetStateTreeComponentForAI(const ATATCharacterAIBase* aiCharacter, const FString& contextName)
   {
      if (!ensureMsgf(aiCharacter != nullptr, TEXT("%s called with a null character!"),
         *contextName))
      {
         return nullptr;
      }

      const AAIController* aiController = Cast<AAIController>(aiCharacter->GetController());
      if (!ensureMsgf(aiController != nullptr, TEXT("%s unable to find controller for %s!"),
         *contextName, *aiCharacter->GetName()))
      {
         return nullptr;
      }

      return Cast<UStateTreeComponent>(aiController->GetBrainComponent());
   }
}

void UTATAIReactionCoordinatorSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   // Kick off async loading of the event configurations table so it's data is available
   // without waiting for its immediate need.
   UTATAIReactionSettings::GetMutable().LoadEventConfigurationsAsync();
}

bool UTATAIReactionCoordinatorSubsystem::RegisterAIForStimTarget(ATATCharacterAIBase* aiCharacter, const FStimInfo& localStim, FGameplayTag& outRegisteredRole)
{
   const FTATAIReactionTarget target = FTATAIReactionTarget::GenerateForStim(aiCharacter, localStim);
   if (target.IsValid())
   {
      return RegisterAIForTarget(aiCharacter, target, outRegisteredRole);
   }
   return false;
}

bool UTATAIReactionCoordinatorSubsystem::RegisterAIForActorTarget(ATATCharacterAIBase* aiCharacter, const AActor* actor, FGameplayTag& outRegisteredRole)
{
   const FTATAIReactionTarget target = FTATAIReactionTarget::GenerateForActor(actor);
   if (target.IsValid())
   {
      return RegisterAIForTarget(aiCharacter, target, outRegisteredRole);
   }
   return false;
}

bool UTATAIReactionCoordinatorSubsystem::UnregisterActorForStimTarget(ATATCharacterAIBase* aiCharacter, const FStimInfo& localStim)
{
   const FTATAIReactionTarget target = FTATAIReactionTarget::GenerateForStim(aiCharacter, localStim);
   if (target.IsValid())
   {
      return UnregisterAIForTarget(aiCharacter, target);
   }
   return false;
}

bool UTATAIReactionCoordinatorSubsystem::UnregisterActorForActorTarget(ATATCharacterAIBase* aiCharacter, const AActor* actor)
{
   const FTATAIReactionTarget target = FTATAIReactionTarget::GenerateForActor(actor);
   if (target.IsValid())
   {
      return UnregisterAIForTarget(aiCharacter, target);
   }
   return false;
}

const FTATAIReactionRole::FRegistrationRoster* UTATAIReactionCoordinatorSubsystem::RetrieveRoleRosterForQuery(int32 queryId)
{
   const FTATAIReactionRoleEvaluatorId evaluatorId(queryId);
   if (const FTATAIReactionRoleId* roleId = _evaluatorsInFlight.Find(evaluatorId))
   {
      if (const FTATAIReactionRole* role = _GetEventRole(*roleId))
      {
         return &role->GetRoster();
      }
   }

   UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] RetrieveRoleRosterForQuery called with a queryId for which no role can be found!"),
      *GetName());
   return nullptr;
}

bool UTATAIReactionCoordinatorSubsystem::RetrieveTargetLocationForQuery(int32 queryId, bool useProjectedLocation, FVector& targetLocation) const
{
   const FTATAIReactionRoleEvaluatorId evaluatorId(queryId);
   if (const FTATAIReactionRoleId* roleId = _evaluatorsInFlight.Find(evaluatorId))
   {
      targetLocation = useProjectedLocation ? roleId->Target.GetNavMeshProjectedLocation(this) : roleId->Target.GetLocation();
      return true;
   }

   UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] RetrieveTargetLocationForQuery called with a queryId for which no role can be found!"),
      *GetName());
   targetLocation = FVector::ZeroVector;
   return false;
}

bool UTATAIReactionCoordinatorSubsystem::IsAIRegisteredWithEvent(const ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target) const
{
   if (const FTATAIReactionTarget* existingTarget = _registeredAI.Find(aiCharacter))
   {
      return (target == *existingTarget);
   }

   return false;
}

bool UTATAIReactionCoordinatorSubsystem::IsAIRegisteredForRole(const ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target,
   const FGameplayTag& roleTag) const
{
   const FTATAIReactionRoleId roleId(target, roleTag);
   if (const FTATAIReactionRole* role = _GetEventRole(roleId))
   {
      return role->GetRoster().Registrants.Contains(aiCharacter);
   }

   return false;
}

bool UTATAIReactionCoordinatorSubsystem::IsRoleAvailableOrClaimedByAI(const FTATAIReactionTarget& target, const FGameplayTag& roleTag, 
   const ATATCharacterAIBase* aiCharacter, bool& outEventConfigExists)
{
   // Returns true if this event config contains the specified role and if
   // an event would survive if the specified role was added.
   auto checkEventConfig = [](const FTATAIReactionEventConfig* eventConfig, const FGameplayTag& roleTag, 
      const int32 numConditionalRolesAssigned) -> bool
   {
      // If there are no conditional roles assigned and this event should not continue
      // if no conditional roles are assigned, we don't want a new event to start if
      // asked about an AI's ability to register for a default role.
      if (eventConfig->EndEventWhenNoConditionalRolesAssigned && numConditionalRolesAssigned == 0
         && eventConfig->DefaultRole.MatchesTagExact(roleTag))
      {
         return false;
      }
      else
      {
         return eventConfig->ContainsRole(roleTag);
      }
   };

   if (const FTATAIReactionEvent* reactionEvent = _GetEvent(target))
   {
      // If an event of this type has already been created, it'll have a config.
      outEventConfigExists = true;

      if (const FTATAIReactionRole* role = reactionEvent->GetRole(roleTag))
      {
         const bool isRegistrant = role->GetRoster().Registrants.Contains(aiCharacter);
         const bool isAtMaxRegistrants = (role->GetNumRegistrantOpenings() == 0);
         return isRegistrant || !isAtMaxRegistrants;
      }

      const FTATAIReactionEventConfig* eventConfig = reactionEvent->GetConfig();
      check(eventConfig != nullptr);
      const int32 numConditionalRolesAssigned = reactionEvent->GetNumConditionalRoleRegistrants();
      return checkEventConfig(eventConfig, roleTag, numConditionalRolesAssigned);
   }
   else
   {
      // If the role instance isn't created, it's possible that no one has yet attempted to register for the role.
      // Check the event config to see if the role exists.
      const UTATAIReactionSettings& reactionSettings = UTATAIReactionSettings::Get();
      const FTATAIReactionEventConfigId eventConfigId = reactionSettings.FindEventConfigIdForTarget(target);
      const FTATAIReactionEventConfig* eventConfig = reactionSettings.GetEventConfig(eventConfigId);
      if (eventConfig != nullptr)
      {
         outEventConfigExists = eventConfig->IsEnabled;
         constexpr int32 numConditionalRolesAssigned = 0;
         return outEventConfigExists && checkEventConfig(eventConfig, roleTag, numConditionalRolesAssigned);
      }
   }

   UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] IsRoleAvailableOrClaimedByAI called for a role (%s)/event (%s) that could not be found for (%s)!"),
      *GetName(), *target.ToString(), *roleTag.GetTagName().ToString(), *GetNameSafe(aiCharacter));
   outEventConfigExists = false;
   return false;
}

const FTATAIReactionEvent* UTATAIReactionCoordinatorSubsystem::_GetEvent(const FTATAIReactionTarget& target) const
{
   return _registeredEvents.FindByHash(GetTypeHash(target), target);
}

const FTATAIReactionRole* UTATAIReactionCoordinatorSubsystem::_GetEventRole(const FTATAIReactionRoleId& roleId) const
{
   if (const FTATAIReactionEvent* reactionEvent = _GetEvent(roleId.Target))
   {
      return reactionEvent->GetRole(roleId.RoleTag);
   }

   return nullptr;
}

bool UTATAIReactionCoordinatorSubsystem::RegisterAIForTarget(ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target, FGameplayTag& outRegisteredRole)
{
   if(target.IsValid() == false)
      return false;
   
   // Is this AI already registered for an event?
   if (const FTATAIReactionTarget* existingTarget = _registeredAI.Find(aiCharacter))
   {
      // AI are only allowed to be registered for one event/role, so if we get a 
      // registration request when the AI is already registered, that's a failure.
      // Could someday consider automatic re-registering, but onerous for unregistering
      // should fall on the caller which registered in the first place.
      UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] %s attempted to register for a target (%s) but was already registered for an event (%s)!"),
         *GetName(), *GetNameSafe(aiCharacter), *target.ToString(), *existingTarget->ToString());
      outRegisteredRole = FGameplayTag::EmptyTag;
      return false;
   }

   // Does an event reaction actually exist for this type of target?
   FTATAIReactionEventConfigId eventConfigId = UTATAIReactionSettings::Get().FindEventConfigIdForTarget(target);
   if (!eventConfigId.IsValid())
   {
      // No event reaction (config) has been authored for this type of target.
      UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] %s attempted to register for a target (%s) that does not have an authored event!"),
         *GetName(), *GetNameSafe(aiCharacter), *target.ToString());
      outRegisteredRole = FGameplayTag::EmptyTag;
      return false;
   }

   // If the id was valid, the config should be too.
   const FTATAIReactionEventConfig* eventConfig = UTATAIReactionSettings::Get().GetEventConfig(eventConfigId);
   check(eventConfig != nullptr);
   if (!eventConfig->IsEnabled)
   {
      outRegisteredRole = FGameplayTag::EmptyTag;
      return false;
   }

   // Create the reaction event if it doesn't already exist.
   const float currentTime = GetWorld()->GetTimeSeconds();
   FTATAIReactionEvent* reactionEvent = _GetEvent(target);
   if (reactionEvent == nullptr)
   {
      FSetElementId setId = _registeredEvents.Emplace({ target, eventConfigId, currentTime });
      reactionEvent = &_registeredEvents.Get(setId);
   }

   FTATAIReactionRegistrationContext context(target, aiCharacter, currentTime);
   const bool success = _RegisterAIForEvent(context, *reactionEvent);

   if (context.Out.State != ETATAIReactionRegistrationState::None)
   {
      // So long as registration didn't fail, let's track the registered AI.
      _TrackAIForTarget(aiCharacter, target);

      const FTATAIReactionRoleId newRoleId(target, context.Out.RoleTag);
      const FTATAIReactionRoleId oldRoleId(target, FGameplayTag::EmptyTag);
      _SendEventForAIRoleChange(aiCharacter, newRoleId, oldRoleId);
   }

   outRegisteredRole = context.Out.RoleTag;
   
   UE_LOG(LogTATAIReactionCoordinator, Verbose, TEXT("[%s] RegisterAIForTarget  succeeded for [%s] - Target [%s] - Role - [%s]"),
      *GetName(),
      *GetNameSafe(context.In.AICharacter),
      *target.ToString(),
      *outRegisteredRole.ToString()
      );
   return success;
}

bool UTATAIReactionCoordinatorSubsystem::_RegisterAIForEvent(FTATAIReactionRegistrationContext& context, FTATAIReactionEvent& reactionEvent)
{
   if (!context.In.IsValid())
   {
      return false;
   }

   reactionEvent.RegisterAI(context);

   if (context.Out.State == ETATAIReactionRegistrationState::Overflow)
   {
      // If the registering AI was just put into overflow, make sure
      // we're evaluating overflow.
      _UpdateEventOverflow(context.Target);
   }
   return true;
}

bool UTATAIReactionCoordinatorSubsystem::UnregisterAIForTarget(ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target)
{
   // Does an event reaction actually exist for this type of target?
   FTATAIReactionEventConfigId eventConfigId = UTATAIReactionSettings::Get().FindEventConfigIdForTarget(target);
   if (!eventConfigId.IsValid())
   {
      // No event reaction (config) has been authored for this type of target.
      UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] %s attempted to unregister for a target (%s) that does not have an authored event!"),
         *GetName(), *GetNameSafe(aiCharacter), *target.ToString());
      return false;
   }

   FTATAIReactionEvent* reactionEvent = _GetEvent(target);
   if (reactionEvent == nullptr)
   {
      UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] %s attempted to unregister for a target (%s) that does not have an reaction event!"),
         *GetName(), *GetNameSafe(aiCharacter), *target.ToString());
      return false;
   }

   if(_UntrackAIForTarget(aiCharacter, target) == false)
   {
      UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] %s attempted to unregister for a target (%s) that was never tracked to begin with"),
         *GetName(), *GetNameSafe(aiCharacter), *target.ToString());
      return false;
   }
   
   const float currentTime = GetWorld()->GetTimeSeconds();
   FTATAIReactionRegistrationContext context(target, aiCharacter, currentTime);
   reactionEvent->UnregisterAI(context);

   switch (context.Out.State)
   {
      case ETATAIReactionRegistrationState::Registrant:
      {
         const FTATAIReactionRoleId newRoleId(target, FGameplayTag::EmptyTag);
         const FTATAIReactionRoleId oldRoleId(target, context.Out.RoleTag);
         if (!aiCharacter->IsActorBeingDestroyed())
         {
            _SendEventForAIRoleChange(aiCharacter, newRoleId, oldRoleId);
         }

         _TryEvaluateRoleOverflow(FTATAIReactionRoleId(target, context.Out.RoleTag));
         _CheckForEventEnd(target);

         UE_LOG(LogTATAIReactionCoordinator, Verbose, TEXT("[%s] UnregisterAIForTarget succeeded for [%s] - Target [%s]"),
            *GetName(),
            *GetNameSafe(aiCharacter),
            *target.ToString()
         );
         return true;
      }

      case ETATAIReactionRegistrationState::Overflow:
      {
         // For some reasons, the AI wasn't a registrant for any role but was found
         // in a role's overflow list...
         UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] %s attempted to unregister for a target (%s) but was only found as overflow!"),
            *GetName(), *GetNameSafe(aiCharacter), *target.ToString());
         return false;
      }

      case ETATAIReactionRegistrationState::None:
      {
         UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] %s attempted to unregister for a target (%s) but was not found in any role rosters!"),
            *GetName(), *GetNameSafe(aiCharacter), *target.ToString());
         return false;
      }

      default:
      {
         checkNoEntry();
         return false;
      }
   }
}

void UTATAIReactionCoordinatorSubsystem::_TrackAIForTarget(const ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target)
{
   check(!_registeredAI.Contains(aiCharacter));

   _registeredAI.Add(aiCharacter, target);

   // Track if the registered AI is destroyed - we'll need to unregister them if so.
   const_cast<ATATCharacterAIBase*>(aiCharacter)->OnDestroyed.AddDynamic(this, &UTATAIReactionCoordinatorSubsystem::_OnRegisteredAIDestroyed);
}

bool UTATAIReactionCoordinatorSubsystem::_UntrackAIForTarget(const ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target)
{
   const FTATAIReactionTarget* foundValue = _registeredAI.Find(aiCharacter);
   if(foundValue == nullptr)
      return false;
   
   if(*foundValue == target)
   {
      _registeredAI.Remove(aiCharacter);

      // No need to track the AI's destruction if they're no longer assigned to a reaction event.
      const_cast<ATATCharacterAIBase*>(aiCharacter)->OnDestroyed.RemoveDynamic(this, &UTATAIReactionCoordinatorSubsystem::_OnRegisteredAIDestroyed);
      return true;
   }
   return false;
}

void UTATAIReactionCoordinatorSubsystem::_SendEventForAIRoleChange(const ATATCharacterAIBase* aiCharacter, 
   const FTATAIReactionRoleId& newRoleId, const FTATAIReactionRoleId& oldRoleId)
{
   UStateTreeComponent* stateTreeComponent = TATAIReactionCoordinatorHelpers::GetStateTreeComponentForAI(aiCharacter, TEXT("__SendEventForAIRoleChange"));
   if (!ensureMsgf(stateTreeComponent != nullptr, TEXT("_SendEventForAIRoleChange unable to find state tree component for %s!"),
         *aiCharacter->GetName()))
   {
      return;
   }

   FTATAITargetingEvent_ReactionRoleChanged eventPayload;
   eventPayload.Target = const_cast<ATATCharacterAIBase*>(aiCharacter);
   eventPayload.NewRoleTag = newRoleId.RoleTag;
   eventPayload.OldRoleTag = oldRoleId.RoleTag;

   FStateTreeEvent event;
   event.Tag = TAG_StateTreeEvent_ReactionRoleChange;
   event.Payload = FInstancedStruct::Make(eventPayload);

   stateTreeComponent->SendStateTreeEvent(event);
   
   UE_LOG(LogTATAIReactionCoordinator, Verbose, TEXT("[%s] role changed from [%s] to [%s]"),
      *aiCharacter->GetName(),
      *oldRoleId.RoleTag.ToString(),
      *newRoleId.RoleTag.ToString());
}

void UTATAIReactionCoordinatorSubsystem::_SendEventForAIEventEnded(const ATATCharacterAIBase* aiCharacter, const FTATAIReactionEvent* endingEvent)
{
   UStateTreeComponent* stateTreeComponent = TATAIReactionCoordinatorHelpers::GetStateTreeComponentForAI(aiCharacter, TEXT("_SendEventForAIEventEnded"));
   if (!ensureMsgf(stateTreeComponent != nullptr, TEXT("_SendEventForAIRoleChange unable to find state tree component for %s!"),
      *aiCharacter->GetName()))
   {
      return;
   }

   FStateTreeEvent event;
   event.Tag = TAG_StateTreeEvent_ReactionEventEnd;

   stateTreeComponent->SendStateTreeEvent(event);
}

void UTATAIReactionCoordinatorSubsystem::_CheckForEventEnd(const FTATAIReactionTarget& target)
{
   const FTATAIReactionEvent* reactionEvent = _GetEvent(target);
   if (reactionEvent != nullptr && reactionEvent->IsFinished())
   {
      // Notify all current registrants that the event is about to end.
      for (auto itRole = reactionEvent->CreateConstRoleIterator(); itRole; ++itRole)
      {
         const FTATAIReactionRole& role = itRole.Value();
         for (TWeakObjectPtr<const ATATCharacterAIBase> registrant : role.GetRoster().Registrants)
         {
            if (const ATATCharacterAIBase* aiCharacter = registrant.Get())
            {
               // No need to go through the full unregister process (which could trigger a bunch of
               // checks/events unnecessary when tearing down an event), but we do need to make sure
               // we don't retain references to a previous registration. AI should only ever be
               // registered to one event at a time, so if an event they're registered to is ending
               // they'll no longer be registered to anything.
               if(_UntrackAIForTarget(aiCharacter, target))
               {
                  _SendEventForAIEventEnded(aiCharacter, reactionEvent);
               }
            }
         }
      }

      _registeredEvents.Remove(*reactionEvent);
      reactionEvent = nullptr;
   }
}

void UTATAIReactionCoordinatorSubsystem::_UpdateEventOverflow(const FTATAIReactionTarget& target)
{
   const FTATAIReactionEvent* reactionEvent = _GetEvent(target);
   if (reactionEvent == nullptr)
   {
      // Error/warning is probably unnecessary in case EQS finishes after event has completed.
      return;
   }

   if (reactionEvent->HasEvaluatorInFlight())
   {
      // Shouldn't actually be an issue, but the timing is suspect and should be investigated.
      UE_LOG(LogTATAIReactionCoordinator, Warning, TEXT("_UpdateEventOverflow called while an evaluator on %s was already in flight."),
         *target.ToString());
      return;
   }

   // Iterate through all the events roles in priority order to run an overflow
   // evaluator on the highest priority role.
   for (auto itRole = reactionEvent->CreateConstRoleIterator(); itRole; ++itRole)
   {
      const FGameplayTag& roleTag = itRole.Key();

      const FTATAIReactionRoleId roleId(reactionEvent->Target, roleTag);
      _TryEvaluateRoleOverflow(roleId);

      // Has a new evaluator been started up?
      // If so, then we're done here.
      if (reactionEvent->HasEvaluatorInFlight())
      {
         return;
      }
   }

   // If no evaluators are in flight, check to see if there is any registrants/overflow.
   // If not, the event can be ended.
   _CheckForEventEnd(target);
}

void UTATAIReactionCoordinatorSubsystem::_TryEvaluateRoleOverflow(const FTATAIReactionRoleId& roleId)
{
   FTATAIReactionRole* role = _GetEventRole(roleId);
   if (role == nullptr)
   {
      UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("_TryEvaluateRoleOverflow called on a role (%s) that could not be found!"),
         *roleId.ToString());
      return;
   }

   if (role->IsDefaultRole())
   {
      // Role is a default role, there shouldn't ever be overflow.
      return;
   }

   const FTATAIReactionRole::FRegistrationRoster& roleRoster = role->GetRoster();
   
   const int32 overflowCount = roleRoster.Overflow.Num();
   if (overflowCount == 0)
   {
      // If there is no overflow, then there's no registrant shuffling to worry about.
      return;
   }

   const int32 openings = role->GetNumRegistrantOpenings();
   if (openings - overflowCount <= 0)
   {
      // If we're at max registrants and have overflow
      // OR
      // if we have less open registrant slots than overflow applicants
      // let's run an EQS evaluator to determine who is the best fit for the role.
      _ExecuteRoleEvaluator(roleId);
   }
   else if (openings >= overflowCount)
   {
      // If we have enough open registrant slots for all overflow we can just move
      // the overflow over.

      FTATAIReactionEvent* roleEvent = _GetEvent(roleId.Target);
      check(roleEvent);

      const float currentTime = GetWorld()->GetTimeSeconds();
      // roleEvent->UnregisterAI can result in modifying Overflow, so we must iterate over a copy
      for (TWeakObjectPtr<const ATATCharacterAIBase> overflowPtr : CopyTemp(roleRoster.Overflow))
      {
         if (const ATATCharacterAIBase* aiCharacter = overflowPtr.Get())
         {
            FTATAIReactionRegistrationContext context(roleId.Target, aiCharacter, currentTime, role->ConfigId);

            // First, unregister the AI with any lower priority roles they may be affiliated with.
            // This will clear any overflow on other roles as well as registrant status.
            roleEvent->UnregisterAI(context);

            // Ensure we've actually been unregistered.
            check(context.Out.State == ETATAIReactionRegistrationState::Registrant);

            // Cache the role unregistered with before the context is overwritten.
            const FTATAIReactionRoleId oldRoleId(roleId.Target, context.Out.RoleTag);

            // Second, register them with this role.
            roleEvent->RegisterAI(context);

            // Ensure we've been moved into the registration list for the correct role.
            check(roleId.RoleTag == context.Out.RoleTag);

            _SendEventForAIRoleChange(aiCharacter, roleId, oldRoleId);
         }
         else
         {
            UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] _TryEvaluateRoleOverflow found a role (%s) overflow member who was null!"),
               *GetName(), *roleId.ToString());
         }
      }
   }
}

bool UTATAIReactionCoordinatorSubsystem::_ExecuteRoleEvaluator(const FTATAIReactionRoleId& roleId)
{
   FTATAIReactionEvent* reactionEvent = _GetEvent(roleId.Target);
   if (reactionEvent == nullptr)
   {
      UE_LOG(LogTATAIReactionCoordinator, Warning, TEXT("_ExecuteRoleEvaluator failed to find the event associated with (%s)!"),
         *roleId.ToString());
      return false;
   }

   const FTATAIReactionRole* reactionRole = reactionEvent->GetRole(roleId.RoleTag);
   if (reactionRole == nullptr)
   {
      UE_LOG(LogTATAIReactionCoordinator, Warning, TEXT("_ExecuteRoleEvaluator failed to find the role Id (%s)!"),
         *roleId.ToString());
      return false;
   }

   const FTATAIConditionalReactionRoleConfig* roleConfig = reactionRole->GetConfig();
   if (roleConfig == nullptr)
   {
      UE_LOG(LogTATAIReactionCoordinator, Warning, TEXT("_ExecuteRoleEvaluator failed to find the role config associated with (%s)!"),
         *roleId.ToString());
      return false;
   }

   // Kick off the EQS query and cache its Id for reference.
   FEnvQueryRequest request(roleConfig->RegistrantScoringQuery);
   const int32 queryId = request.Execute(EEnvQueryRunMode::Type::AllMatching, this, &UTATAIReactionCoordinatorSubsystem::_RoleEvaluatorQueryFinished);

   const FTATAIReactionRoleEvaluatorId evaluatorId(queryId);
   _evaluatorsInFlight.Add(evaluatorId, roleId);
   reactionEvent->SetEvaluatorIdInFlight(evaluatorId);
   return true;
}

void UTATAIReactionCoordinatorSubsystem::_RoleEvaluatorQueryFinished(TSharedPtr<FEnvQueryResult> queryResult)
{
   if (!queryResult.IsValid())
   {
      UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("_RoleEvaluatorQueryFinished called but queryResults was not valid!"));
      return;
   }
   
   if (!queryResult->IsSuccessful())
   {
      UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("_RoleEvaluatorQueryFinished called but query failed (%s)!"),
         *UEnum::GetValueAsString(queryResult->GetRawStatus()));
      return;
   }

   // Local copy so we don't lose the info when it's removed from the map.
   const FTATAIReactionRoleEvaluatorId evaluatorId(queryResult->QueryID);
   const FTATAIReactionRoleId& eventRoleId = _evaluatorsInFlight.FindChecked(evaluatorId);
   const FTATAIReactionTarget target = eventRoleId.Target;

   const float currentTime = GetWorld()->GetTimeSeconds();
   FTATAIReactionRoleEvaluatorResults results(eventRoleId, currentTime);

   // FEnvQueryInstance::FinalizeQuery should auto-sort items by score
   queryResult->GetAllAsActors(results.In.QueryScoreSortedAI);

   if (FTATAIReactionEvent* reactionEvent = _GetEvent(target))
   {
      reactionEvent->ApplyEvaluatorResults(results);

      for (FTATAIRegisteredRoleChangeData& newRegistrant : results.Out.NewRegistrants)
      {
         if (const ATATCharacterAIBase* aiCharacter = newRegistrant.AICharacter.Get())
         {
            const FTATAIReactionRoleId newRoleId(target, newRegistrant.NewRole);
            const FTATAIReactionRoleId oldRoleId(target, newRegistrant.OldRole);
            _SendEventForAIRoleChange(aiCharacter, newRoleId, oldRoleId);
         }
      }
      for (FTATAIRegisteredRoleChangeData& orphan : results.Out.Orphans)
      {
         if (const ATATCharacterAIBase* aiCharacter = orphan.AICharacter.Get())
         {
            const FTATAIReactionRoleId newRoleId(target, orphan.NewRole);
            const FTATAIReactionRoleId oldRoleId(target, orphan.OldRole);
            _SendEventForAIRoleChange(aiCharacter, newRoleId, oldRoleId);
         }
      }

      // Remove the evaluator from the set, now that it is complete.
      _evaluatorsInFlight.Remove(evaluatorId);
      reactionEvent->ClearEvaluatorIdInFlight();

      // Check if there is any more overflow within this event.
      // It is possible some was added after the EQS query was started.
      _UpdateEventOverflow(target);
   }
   else
   {
      // Remove the evaluator from the set, now that it is complete.
      _evaluatorsInFlight.Remove(evaluatorId);
   }
}

void UTATAIReactionCoordinatorSubsystem::_OnRegisteredAIDestroyed(AActor* actor)
{
   ATATCharacterAIBase* aiCharacter = CastChecked<ATATCharacterAIBase>(actor);

   // If this bound event was called, the AI should still be registered within the subsystem.
   if (const FTATAIReactionTarget* target = _registeredAI.Find(aiCharacter))
   {
      UnregisterAIForTarget(aiCharacter, *target);
   }
   else
   {
      UE_LOG(LogTATAIReactionCoordinator, Error, TEXT("[%s] _OnRegisteredAIDestroyed called for an actor (%s) not registered!"),
         *GetName(), *actor->GetName());
   }
}
