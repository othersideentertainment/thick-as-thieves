// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Reactions/TATAIReactionEvaluator.h"
#include "AI/Reactions/TATAIReactionHelpers.h"

// ue
#include "GameplayTagContainer.h"
#include "Containers/Set.h"
#include "UObject/ObjectPtr.h"
#include "UObject/SoftObjectPtr.h"

#include "TATAIReactionRole.generated.h"

class ATATCharacterAIBase;
class UEnvQuery;
struct FTATAIReactionEvent;

// Content authoring capability for defining conditions that must be met for an AI to be registered
// to a reaction role within a reaction event.
USTRUCT(BlueprintType)
struct FTATAIConditionalReactionRoleConfig
{
   GENERATED_BODY()

   // Tag specifying the role.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (Categories = "AI.Behavior.Role"))
   FGameplayTag RoleTag;

   // Optional timing window, relative to the creation of the associated Reaction Event,
   // that AI can potentially register for this role.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float RegistrationGracePeriod = 0.0f;

   // Optional max number of AI that can register for this role.
   // Zero or negative values indicate no max limit.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   int32 MaxRegistrants = 0;

   // When MaxRegistrants is positive, this query is used to score
   // overflow registrants to determine those "best fit" for this role.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "MaxRegistrants > 0"))
   TObjectPtr<UEnvQuery> RegistrantScoringQuery;
};

// Instance of a role within a reaction event. Manages the AI that are and can be
// assigned to this role, as defined within the associated FTATAIConditionalReactionRoleConfig.
struct FTATAIReactionRole
{
public:
   // Store the registrants and overflow together to more easily pass along the data
   // to whoever needs it (i.e. role evaluators).
   struct FRegistrationRoster
   {
      // The set of AI that have been assigned this role.
      TSet<TWeakObjectPtr<const ATATCharacterAIBase>> Registrants;

      // When the number of registrants hits the maximum number allowed by this role,
      // AI are then added here to be considered for taking a registrant slot if they
      // are deemed a "better fit" than someone currently registered.
      TSet<TWeakObjectPtr<const ATATCharacterAIBase>> Overflow;
   };

   FTATAIReactionRole() { }
   FTATAIReactionRole(const FTATAIReactionEventConfigId& owningEventConfigId, const FTATAIConditionalReactionRoleConfigId& configId)
      : ConfigId(configId), _owningEventConfigId(owningEventConfigId){ }

   // Useful for quick lookup of the associated reaction role within the owning
   // reaction event configurtation.
   const FTATAIConditionalReactionRoleConfigId ConfigId;

   // Attempts to register an AI with this role, capacity allowing. Returns the
   // registration status of the AI with this role.
   ETATAIReactionRegistrationState TryRegisterAI(FTATAIReactionRegistrationContext& context, const FTATAIReactionEvent& owningEvent);

   // Attempts to unregister an AI with this role, if it were previously a registrant
   // or an overflow. Returns which status the AI had with this role, if any.
   ETATAIReactionRegistrationState TryUnregisterAI(FTATAIReactionRegistrationContext& context);

   int32 GetNumRegistrantOpenings() const;

   // If we don't have a config, that means we're a default role.
   FORCEINLINE bool IsDefaultRole() const { return !ConfigId.IsValid(); }

   // Retrieve the configuration details for this reaction role which define
   // how AI can/cannot be assigned this role.
   const FTATAIConditionalReactionRoleConfig* GetConfig() const;

   FORCEINLINE const FRegistrationRoster& GetRoster() const { return _roster; }

   // Polls the results of an evaluator on this role and shuffles the registrants based
   // on who the evaluator deemed "best fit".
   void ApplyEvaluatorResults(FTATAIReactionRoleEvaluatorResults& results);

private:
   // Cache the owning event's config Id so we can lookup our own config Id.
   const FTATAIReactionEventConfigId _owningEventConfigId;

   // Contains the sets of AI that have been assigned this role or have been marked
   // overflow and may eventually become a registrant.
   FRegistrationRoster _roster;
};
