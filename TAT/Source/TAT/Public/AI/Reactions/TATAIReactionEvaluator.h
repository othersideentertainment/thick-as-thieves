// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Reactions/TATAIReactionHelpers.h"

// ue
#include "Containers/Array.h"
#include "Containers/Set.h"
#include "UObject/WeakObjectPtrTemplates.h"

class AActor;
class ATATCharacterAIBase;

struct FTATAIReactionRoleEvaluatorId
{
public:
   FTATAIReactionRoleEvaluatorId() = default;
   explicit FTATAIReactionRoleEvaluatorId(const int32 queryId) : QueryId(queryId) { }
   explicit FTATAIReactionRoleEvaluatorId(const FTATAIReactionRoleEvaluatorId& other) : QueryId(other.QueryId) { }

   FORCEINLINE void Invalidate() { QueryId = INDEX_NONE; }

   FORCEINLINE bool IsValid() const { return QueryId != INDEX_NONE; }
   FORCEINLINE bool operator==(const FTATAIReactionRoleEvaluatorId& other) const { return QueryId == other.QueryId; }

   friend uint32 GetTypeHash(const FTATAIReactionRoleEvaluatorId& evaluatorId)
   {
      return GetTypeHash(evaluatorId.QueryId);
   }

private:
   int32 QueryId = INDEX_NONE;

};

struct FTATAIRegisteredRoleChangeData
{
public:
   FTATAIRegisteredRoleChangeData(const ATATCharacterAIBase* aiCharacter) 
      : AICharacter(aiCharacter) { }

   const TWeakObjectPtr<const ATATCharacterAIBase> AICharacter = nullptr;

   // The previous role tag this AI was registered with.
   FGameplayTag OldRole;

   // The current/new role tag this AI is registered with.
   FGameplayTag NewRole;

   bool operator==(const FTATAIRegisteredRoleChangeData& other) const { return AICharacter == other.AICharacter; }

   friend uint32 GetTypeHash(const FTATAIRegisteredRoleChangeData& roleChangeData)
   {
      return GetTypeHash(roleChangeData.AICharacter);
   }
};

// Data passed between the coordinator, event, and roles to shuffle
// role registrants based on an EQS query that determined who was
// found to be "best fit" for a role.
struct FTATAIReactionRoleEvaluatorResults
{
public:
   FTATAIReactionRoleEvaluatorResults(const FTATAIReactionRoleId& roleId, float currentTime) 
      : RoleId(roleId), In({ currentTime }) { }

   const FTATAIReactionRoleId RoleId;

   struct
   {
      // Time at which the associated EQS evaluator finished and this results
      // struct was generated.
      const float CurrentTime = -1.0f;

      // Output of the evaluator query, sorted by highest score, descending.
      // Stored as actors as thats the output of the EQS query.
      // Will not be cached - should only exist for the scope of the EQS
      // query callback function.
      TArray<AActor*> QueryScoreSortedAI;
   } In;

   struct
   {
      // Set of AI that were previously registered to the role associated with
      // RoleId who, after this EQS evaluator was run, are no longer and need
      // to have new roles found for.
      TSet<FTATAIRegisteredRoleChangeData> Orphans;

      // Set of AI that were previously overflowed to the role associated with
      // RoleId who, after this EQS evaluator was run, are now registrants.
      // Will need to be unregistered from any lower priority roles previously
      // registered with.
      TSet<FTATAIRegisteredRoleChangeData> NewRegistrants;
   } Out;
};
