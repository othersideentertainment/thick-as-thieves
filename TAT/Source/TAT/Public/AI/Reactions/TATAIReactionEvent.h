// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Reactions/TATAIReactionEvaluator.h"
#include "AI/Reactions/TATAIReactionHelpers.h"
#include "AI/Reactions/TATAIReactionRole.h"
#include "AI/Reactions/TATAIReactionTarget.h"

// ue
#include "GameplayTagContainer.h"
#include "Containers/Array.h"
#include "Containers/Map.h"

#include "TATAIReactionEvent.generated.h"

// Content authoring capability for defining reaction events - occurrences which AI wish to react to,
// and can fall into one of many roles in order to accomplish vbarious tasks towards a common goal.
USTRUCT(BlueprintType)
struct TAT_API FTATAIReactionEventConfig : public FTableRowBase
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool IsEnabled = true;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ShowOnlyInnerProperties))
   FTATAIReactionTargetConfig Target;

   // If no ConditionalRoles are specified, or if an AI is not considered a "best fit" for any
   // ConditionalRoles, AI will be assigned this role. No max limit on this role.
   // If set to none, not default role will be created/can be registered with.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "AI.Behavior.Role"))
   FGameplayTag DefaultRole;

   // Optional array of roles containing metadata that filter registrants into roles. The priority for role 
   // assignment is implied in the role's index in the array (the lower the index, the higher the priority). 
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TArray<FTATAIConditionalReactionRoleConfig> ConditionalRoles;

   // If set to true, at least 1 AI needs to be registered for at least 1 conditional role for this
   // event to stay active. Once no conditional roles are assigned, the event will be destroyed.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool EndEventWhenNoConditionalRolesAssigned = false;

   bool ContainsRole(const FGameplayTag& roleTag) const;
};

// Runtime instance of a reaction event. Manages the roles associated with the event and the AI
// that wish to register to react to the event.
struct FTATAIReactionEvent
{
public:
   FTATAIReactionEvent(const FTATAIReactionTarget& target, const FTATAIReactionEventConfigId& configId, const float& startTime)
      : Target(target), ConfigId(configId), StartTime(startTime) { }

   // The actor or hearing stim target of this event.
   const FTATAIReactionTarget Target;

   // Useful for quick lookup of the associated reaction event configurtation.
   const FTATAIReactionEventConfigId ConfigId;

   // The time at which this reaction event instance was created.
   // (i.e. the time of the first AI to register for the associated reaction target)
   const float StartTime = 0.0f;

   void RegisterAI(FTATAIReactionRegistrationContext& context);
   void UnregisterAI(FTATAIReactionRegistrationContext& context);

   bool IsFinished() const;

   FTATAIReactionRole* GetRole(const FGameplayTag& roleTag);
   const FTATAIReactionRole* GetRole(const FGameplayTag& roleTag) const;

   // Retrieve the configuration details for this reaction event which defines
   // the roles associated with the reaction target.
   const FTATAIReactionEventConfig* GetConfig() const;

   // Expose a const interator for this event's roles to allow outsiders to
   // iterate over them without making changes.
   FORCEINLINE TMap<FGameplayTag, FTATAIReactionRole>::TConstIterator CreateConstRoleIterator() const { return _roles.CreateConstIterator(); }

   FORCEINLINE int32 GetNumConditionalRoleRegistrants() const { return _numConditionalRoleRegistrants; }

   FORCEINLINE void SetEvaluatorIdInFlight(const FTATAIReactionRoleEvaluatorId& id) { _evaluatorIdInFlight = id; }
   FORCEINLINE void ClearEvaluatorIdInFlight() { _evaluatorIdInFlight.Invalidate(); }
   FORCEINLINE bool HasEvaluatorInFlight() const { return _evaluatorIdInFlight.IsValid(); }

   void ApplyEvaluatorResults(FTATAIReactionRoleEvaluatorResults& results);

   bool operator==(const FTATAIReactionEvent& reactionEvent) const { return Target == reactionEvent.Target; }
   bool operator==(const FTATAIReactionTarget& reactionTarget) const { return Target == reactionTarget; }

private:
   TMap<FGameplayTag, FTATAIReactionRole> _roles;

   // Cache the query Id of the EQS query currently operating on a role in this event
   // on the event itself. Used as a lookup in the reaction coordinator. Any event
   // should only have one EQS evaluator in flight at any given time.
   FTATAIReactionRoleEvaluatorId _evaluatorIdInFlight;

   // Cache of the number of conditional role registrants (excludes default role
   // registrants) to make the information available to requesters.
   int32 _numConditionalRoleRegistrants = 0;

#if DO_ENSURE
   int32 _CalcNumConditionalRoleRegistrants() const;
#endif
};

inline uint32 GetTypeHash(const FTATAIReactionEvent& reactionEvent)
{
   return GetTypeHash(reactionEvent.Target);
}
