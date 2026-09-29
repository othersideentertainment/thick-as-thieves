// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "AI/Perception/StimInfo.h"
#include "NativeGameplayTags.h"
#include "SmartObjectTypes.h"

#include "TATStateTreeEvents.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_GameplayTagChange)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_ReactionEventEnd)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_ReactionRoleChange)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_StimChange)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_LockdownEvent)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_AbandonLockdown)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_LockdownHoldPosition)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_LockdownNewAlarmGuard)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_LockdownGuardLeaving)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_TargetChange)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_DistractionEvent)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_SharePartnerLost)

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_StateTreeEvent_ClueChange)

USTRUCT()
struct FTATAITargetingEvent_GameplayTagChanged
{
   GENERATED_BODY()

   UPROPERTY()
   bool Exists { true };

   UPROPERTY()
   FGameplayTag Tag;
};

USTRUCT()
struct FTATAITargetingEvent_ReactionRoleChanged
{
   GENERATED_BODY()

   UPROPERTY()
   TObjectPtr<AActor> Target { nullptr };

   UPROPERTY()
   FGameplayTag OldRoleTag;

   UPROPERTY()
   FGameplayTag NewRoleTag;
};

USTRUCT()
struct FTATTargetingEvent_Stim
{
   GENERATED_BODY()

   UPROPERTY()
   FStimInfo Stim;
};

USTRUCT()
struct FTATAITargetingEvent_TargetChanged
{
   GENERATED_BODY()

   UPROPERTY()
   TObjectPtr<AActor> Target { nullptr };

   UPROPERTY()
   FGameplayTag TargetingTag;
};


USTRUCT()
struct FTATAITargetingEvent_DistractionEvent
{
   GENERATED_BODY()

   UPROPERTY()
   FSmartObjectSlotHandle DistractionHandle;
};
