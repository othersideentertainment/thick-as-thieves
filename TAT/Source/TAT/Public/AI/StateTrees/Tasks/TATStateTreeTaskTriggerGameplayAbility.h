// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

// ose
#include "Abilities/OSEAbilityInputBinds.h"

// ue
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbilityTypes.h"

#include "TATStateTreeTaskTriggerGameplayAbility.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskTriggerGameplayAbilityBaseData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   bool EndAbilityOnTaskExit { false };

   UPROPERTY(VisibleAnywhere)
   bool IsFinished { false };

   FGameplayAbilitySpecHandle ActivatedAbilitySpecHandle;

   FDelegateHandle BoundDelegateHandle;
};

USTRUCT()
struct FTATStateTreeTaskTriggerGameplayAbilityByClassData : public FTATStateTreeTaskTriggerGameplayAbilityBaseData
{
   GENERATED_BODY()
   UPROPERTY(EditAnywhere, Category="Context")
   AAIController* AIController { nullptr };

   UPROPERTY(EditAnywhere)
   TSubclassOf<UGameplayAbility> AbilityClassToTrigger;
};

USTRUCT(meta = (DisplayName = "Trigger Gameplay Ability By Class", Category = "TAT|Gameplay Ability"))
struct TAT_API FTATStateTreeTaskTriggerGameplayAbilityByClass : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskTriggerGameplayAbilityByClassData;

   FTATStateTreeTaskTriggerGameplayAbilityByClass() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};

USTRUCT()
struct FTATStateTreeTaskTriggerGameplayAbilityData : public FTATStateTreeTaskTriggerGameplayAbilityBaseData
{
   GENERATED_BODY()
   UPROPERTY(EditAnywhere, Category="Context")
   AAIController* AIController { nullptr };

   UPROPERTY(EditAnywhere)
   EAbilityInputType AbilityInputToTrigger { EAbilityInputType::Ability_01 };
};

USTRUCT(meta = (DisplayName = "Trigger Gameplay Ability By Input", Category = "TAT|Gameplay Ability"))
struct TAT_API FTATStateTreeTaskTriggerGameplayAbility : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskTriggerGameplayAbilityData;
	
   FTATStateTreeTaskTriggerGameplayAbility() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};

USTRUCT()
struct FTATStateTreeTaskTriggerGameplayAbilityByEventData : public FTATStateTreeTaskTriggerGameplayAbilityBaseData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = "Context")
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "In")
   FGameplayTag EventTag = FGameplayTag::EmptyTag;

   UPROPERTY(EditAnywhere, Category = "In")
   FGameplayEventData EventData;
};

USTRUCT(meta = (DisplayName = "Trigger Gameplay Ability By Gameplay Event", Category = "TAT|Gameplay Ability"))
struct TAT_API FTATStateTreeTaskTriggerGameplayAbilityByEvent : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()

   using FInstanceDataType = FTATStateTreeTaskTriggerGameplayAbilityByEventData;

   FTATStateTreeTaskTriggerGameplayAbilityByEvent() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
