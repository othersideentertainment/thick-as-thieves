// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"
#include "AI/Reactions/TATAIReactionTarget.h"
#include "Character/TATCharacterAIBase.h"

#include "TATStateTreeTaskRegisterForRole.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeTaskRegisterForRoleInstanceData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;

   UPROPERTY(EditAnywhere, Category = "In", meta = (RefType = "/Script/OSEAI.StimInfo, /Script/Engine.Actor"))
   FStateTreePropertyRef ReactingToStim;

   // If true, if no reaction event configuration is found for the stim, allow the task
   // to run anyways. Useful in cases where the state containing this task covers
   // both behaviors based on roles as well as those not.
   UPROPERTY(EditAnywhere, Category = "In")
   bool AllowToRunIfNoEventConfigExists = true;

   UPROPERTY(EditAnywhere, Category = "Out", meta = (RefType = "/Script/GameplayTags.GameplayTag"))
   FStateTreePropertyRef ResultRegisteredRoleTag;
};

USTRUCT(meta = (DisplayName = "Register for Reaction Role", Category = "TAT|Event|Extractions"))
struct TAT_API FTATStateTreeTaskRegisterForRole : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskRegisterForRoleInstanceData;

   FTATStateTreeTaskRegisterForRole();
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

private:
   static void AttemptToSetTargetFromContext(FStateTreeExecutionContext& context,
                                             const FInstanceDataType& instanceData,
                                             const ATATCharacterAIBase* aiCharacter,
                                             FTATAIReactionTarget& target);
};
