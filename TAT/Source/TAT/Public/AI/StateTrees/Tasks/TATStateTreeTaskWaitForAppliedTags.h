// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"
#include "UObject/WeakInterfacePtr.h"

#include "TATStateTreeTaskWaitForAppliedTags.generated.h"

class APawn;

UENUM()
enum class ETATStateTreeTaskWaitForType : uint8
{
   // Wait for any application of tags since the start of this task.
   TagsApplied_Any,
   // Wait for the first time tags are applied when previously not since the start of this task.
   TagsApplied_First,
   // Wait for any removal of tags since the start of this task.
   TagsRemoved_Any,
   // Wait for tags to be removed completely (count == 0) since the start of this task.
   TagsRemoved_All
};

USTRUCT()
struct FTATStateTreeTaskWaitForAppliedTagsData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<APawn> AIPawn = nullptr;

   // A collection of tags we wish to track. 'WaitForType' defines how we decide when tags
   // no longer need tracking and is used on them individually, rather than as a set.
   // For instance:
   //    If there exist 2+ tags in 'TagsToWaitFor' and 'WaitForType' is set to TagsRemoved_All,
   //    when the count of the first tag in the container reaches zero, it will no longer be 
   //    tracked and we are one step closer to finishing this task.
   UPROPERTY(EditAnywhere, Category = "In")
   FGameplayTagContainer TagsToWaitFor;

   // Defines the event relating to each tag in 'TagsToWaitFor' that this task will wait for before finishing.
   // This defines how each individual tag in 'TagsToWaitFor' are tracked, rather than the collection as a whole.
   UPROPERTY(EditAnywhere, Category = "In")
   ETATStateTreeTaskWaitForType WaitForTypePerTag = ETATStateTreeTaskWaitForType::TagsRemoved_All;

   struct FTagData
   {
      // The last tracked tag count for an ASC-applied tag.
      int32 Count = 0;
      // The ASC binding to track when the count changes.
      FDelegateHandle Handle;
   };
   TMap<FGameplayTag, FTagData> TagsToWaitForData;

   bool IsFinished = false;
};

USTRUCT()
struct TAT_API FTATStateTreeTaskWaitForAppliedTags : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()

   using FInstanceDataType = FTATStateTreeTaskWaitForAppliedTagsData;
   using FInstanceTagDataType = FTATStateTreeTaskWaitForAppliedTagsData::FTagData;
	
   FTATStateTreeTaskWaitForAppliedTags() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
   virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;
   virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
