// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Reactions/TATAIReactionEvaluator.h"
#include "AI/Reactions/TATAIReactionEvent.h"
#include "AI/Reactions/TATAIReactionHelpers.h"
#include "AI/Reactions/TATAIReactionTarget.h"

// ose
#include "AI/Perception/StimInfo.h"

// ue
#include "GameplayTagContainer.h"
#include "Containers/Map.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATAIReactionCoordinator.generated.h"

class AActor;
class ATATCharacterAIBase;
struct FEnvQueryResult;

// Subsystem responsible for assigning registered AI to roles pertaining to an actor or stim they're
// desiring to react to. Groups ot AI desiring to react to the same actor/stim will be assigned different
// roles depending on "best fit" for a given role, thus separating out tasks to accomplish a larger goal.
// These roles are used to inform the AI the behavior they should enter.
UCLASS(meta = (DisplayName = "AI Reaction Coordinator"))
class TAT_API UTATAIReactionCoordinatorSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   // from USubsystem
   virtual void Initialize(FSubsystemCollectionBase& collection) override;

   bool RegisterAIForTarget(ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target, FGameplayTag& outRegisteredRole);
   bool UnregisterAIForTarget(ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target);

   // Indicates that an AI intends to run a behavior to react to a specified stim. Return type specifies
   // the AI's role in the reaction event. May change depending on others who registered for the same stim.
   UFUNCTION(BlueprintCallable)
   bool RegisterAIForStimTarget(ATATCharacterAIBase* aiCharacter, const FStimInfo& localStim, FGameplayTag& outRegisteredRole);

   // Indicates that an AI intends to run a behavior to react to a specified actor. Return type specifies
   // the AI's role in the reaction event. May change depending on others who registered for the same actor.
   UFUNCTION(BlueprintCallable)
   bool RegisterAIForActorTarget(ATATCharacterAIBase* aiCharacter, const AActor* actor, FGameplayTag& outRegisteredRole);

   // Indicates that an AI no longer intends to run a behavior to react to a specified stim.
   UFUNCTION(BlueprintCallable)
   bool UnregisterActorForStimTarget(ATATCharacterAIBase* aiCharacter, const FStimInfo& localStim);

   // Indicates that an AI no longer intends to run a behavior to react to a specified actor.
   UFUNCTION(BlueprintCallable)
   bool UnregisterActorForActorTarget(ATATCharacterAIBase* aiCharacter, const AActor* actor);

   // Retrieves the registrant/overflow AI list for the associated evaluator EQS query.
   const FTATAIReactionRole::FRegistrationRoster* RetrieveRoleRosterForQuery(int32 queryId);

   // Finds the world location of the actor/stim event target that an evaluator EQS query is targeting.
   bool RetrieveTargetLocationForQuery(int32 queryId, bool useProjectedLocation, FVector& targetLocation) const;

   bool IsAIRegisteredWithEvent(const ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target) const;
   bool IsAIRegisteredForRole(const ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target,
      const FGameplayTag& roleTag) const;

   bool IsRoleAvailableOrClaimedByAI(const FTATAIReactionTarget& target, const FGameplayTag& roleTag, 
      const ATATCharacterAIBase* aiCharacter, bool& outEventConfigExists);

private:
   // All the reaction events that at least 1 AI is registered for.
   TSet<FTATAIReactionEvent> _registeredEvents;

   // Local cache for quick lookup of where AI are registered
   TMap<TWeakObjectPtr<const ATATCharacterAIBase>, FTATAIReactionTarget> _registeredAI;

   // Cache of EQS queries in flight with the roles they're evaluating.
   TMap<FTATAIReactionRoleEvaluatorId, FTATAIReactionRoleId> _evaluatorsInFlight;

   const FTATAIReactionEvent* _GetEvent(const FTATAIReactionTarget& target) const;
   FTATAIReactionEvent* _GetEvent(const FTATAIReactionTarget& target)
   { 
      return const_cast<FTATAIReactionEvent*>(const_cast<const UTATAIReactionCoordinatorSubsystem*>(this)->_GetEvent(target));
   }

   const FTATAIReactionRole* _GetEventRole(const FTATAIReactionRoleId& roleId) const;
   FTATAIReactionRole* _GetEventRole(const FTATAIReactionRoleId& roleId)
   {
      return const_cast<FTATAIReactionRole*>(const_cast<const UTATAIReactionCoordinatorSubsystem*>(this)->_GetEventRole(roleId));
   }

   bool _RegisterAIForEvent(FTATAIReactionRegistrationContext& context, FTATAIReactionEvent& reactionEvent);

   void _TrackAIForTarget(const ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target);
   bool _UntrackAIForTarget(const ATATCharacterAIBase* aiCharacter, const FTATAIReactionTarget& target);

   void _SendEventForAIRoleChange(const ATATCharacterAIBase* aiCharacter, const FTATAIReactionRoleId& newRoleId, const FTATAIReactionRoleId& oldRoleId);
   void _SendEventForAIEventEnded(const ATATCharacterAIBase* aiCharacter, const FTATAIReactionEvent* endingEvent);

   void _CheckForEventEnd(const FTATAIReactionTarget& target);

   // Searches through a reaction event's roles to see if any have overflow.
   // If so, kick off an EQS evaluator for the role.
   void _UpdateEventOverflow(const FTATAIReactionTarget& target);

   // Check if a reaction role has overflow and, if so, evaluate the overflow
   // against the registrants .
   void _TryEvaluateRoleOverflow(const FTATAIReactionRoleId& roleId);

   // Trigger the EQS query associated with the role's config to determine
   // who of the registrant/overflow list should stay/become registrants.
   bool _ExecuteRoleEvaluator(const FTATAIReactionRoleId& roleId);

   // Callback when an evaluator EQS query finishes.
   void _RoleEvaluatorQueryFinished(TSharedPtr<FEnvQueryResult> queryResult);

   UFUNCTION()
   void _OnRegisteredAIDestroyed(AActor* actor);
};
