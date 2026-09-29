// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Reactions/TATAIReactionTarget.h"
#include "Character/TATCharacterAIBase.h"

// ue
#include "GameplayTagContainer.h"
#include "Containers/Array.h"
#include "UObject/ObjectPtr.h"

// Where, in a reaction role's roster, is an AI located (if at all).
enum class ETATAIReactionRegistrationState : uint8
{
   Registrant,
   Overflow,
   None,
};

// Data that makes the role configuration quick to lookup within the associated FTATAIReactionEventConfig.
// Note, this is only pertinent for conditional roles (though default roles may store a dummy Id).
struct FTATAIConditionalReactionRoleConfigId
{
   explicit FTATAIConditionalReactionRoleConfigId(int32 roleIndex = INDEX_NONE) : RoleIndex(roleIndex) { }

   // Index within owning FTATAIReactionEventConfig's ConditionalRoles array.
   const int32 RoleIndex = INDEX_NONE;

   FORCEINLINE bool IsValid() const { return RoleIndex >= 0; }
   FORCEINLINE FTATAIConditionalReactionRoleConfigId GetNext() const { return FTATAIConditionalReactionRoleConfigId(RoleIndex + 1); }

   FORCEINLINE static FTATAIConditionalReactionRoleConfigId First() { return FTATAIConditionalReactionRoleConfigId(0); }
};

// Data that makes the associated configuration quick to lookup within the data table which
// deifnes all the reaction event configurations.
struct FTATAIReactionEventConfigId
{
   explicit FTATAIReactionEventConfigId(const FName tableRowName = NAME_None) : TableRowName(tableRowName) { }

   const FName TableRowName = NAME_None;

   FORCEINLINE bool IsValid() const { return TableRowName != NAME_None; }
};

inline uint32 GetTypeHash(const FTATAIReactionEventConfigId& configId)
{
   return GetTypeHash(configId.TableRowName);
}

// Identifier data useful in the lookup of a reaction role instance (FTATAIReactionRole).
struct FTATAIReactionRoleId
{
public:
   FTATAIReactionRoleId(const FTATAIReactionTarget& target, const FGameplayTag& roleTag)
      : Target(target), RoleTag(roleTag) { }

   // Useful for FTATAIReactionEvent lookup.
   const FTATAIReactionTarget Target;

   // Useful for FTATAIReactionRole lookup within FTATAIReactionEvent.
   const FGameplayTag RoleTag;

   FORCEINLINE bool IsValid() const { return Target.IsValid() && (RoleTag != FGameplayTag::EmptyTag); }

   FORCEINLINE FString ToString() const
   {
      return FString::Printf(TEXT("{ %s | %s }"), *Target.ToString(), *RoleTag.ToString());
   }
};

// Used in the registration of AI with reaction events and reaction roles.
// Useful to contain multiple desired inputs to and outputs from the registration process.
struct FTATAIReactionRegistrationContext
{
   FTATAIReactionRegistrationContext(const FTATAIReactionTarget& target, const ATATCharacterAIBase* aiCharacter, float currentTime,
      const FTATAIConditionalReactionRoleConfigId startingRoleId = FTATAIConditionalReactionRoleConfigId::First())
      : Target(target), In({ aiCharacter, currentTime, startingRoleId }) { }

   const FTATAIReactionTarget Target;

   struct
   {
      const TObjectPtr<const ATATCharacterAIBase> AICharacter = nullptr;
      const float CurrentTime = -1.0f;

      // Optional role to start registration checking with.
      // By default (via optional param in constructor) will start with the first.
      const FTATAIConditionalReactionRoleConfigId StartingRoleId;

      bool IsValid() const { return (AICharacter != nullptr) && (CurrentTime >= 0.0f); }
   } In;

   struct
   {
      FGameplayTag RoleTag;

      // Assume we'll be able to find a role at least with the DefaultRole.
      // Need to set to a different state if something goes wrong.
      ETATAIReactionRegistrationState State = ETATAIReactionRegistrationState::Registrant;
   } Out;
};
