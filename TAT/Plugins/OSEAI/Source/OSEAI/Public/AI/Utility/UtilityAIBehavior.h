// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/UtilityAIState.h"

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "UtilityAIBehavior.generated.h"

class UBehaviorTree;

/// State that is re-instantiated before a behavior is entered
USTRUCT(BlueprintType)
struct OSEAI_API FUtilityAIBehaviorRuntimeState
{
   GENERATED_BODY()

public:
   /// If false, the system will not consider other behaviors until this one completes.
   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility")
   bool IsInterruptable = true;
};

/// A behavior for the utility AI
UCLASS(BlueprintType, Blueprintable)
class OSEAI_API UUtilityAIBehavior : public UUtilityAIStateBase
{
   GENERATED_BODY()

public:

   UUtilityAIBehavior();

   // from UUtilityAIStateBase
   virtual void Init(UUtilityAIComponent& utilityAIComponent) override;
   virtual void Reset() override;
   virtual void Enter() override;
   virtual void Exit() override;
   virtual bool IsInterruptible() const override { return _behaviorState != EBehaviorState::NotInterruptible; }
   virtual bool IsComplete() const override { return _behaviorState == EBehaviorState::Completed; }

   /// Called when the currently executing behavior tree is finished.
   UFUNCTION(BlueprintNativeEvent, Category = "AI|OSE|Utility")
   void OnBehaviorTreeFinished();
   virtual void OnBehaviorTreeFinished_Implementation() {}

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   void Complete();

   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   EBehaviorState GetBehaviorState() const { return _behaviorState; }

protected:

   /// Behavior tree asset to run on Enter of this behavior.
   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility")
   UBehaviorTree* BehaviorTree = nullptr;

   /// Our default runtime state
   UPROPERTY(EditDefaultsOnly, Category = "AI|OSE|Utility")
   FUtilityAIBehaviorRuntimeState DefaultRuntimeState;

private:
   void _ResetRuntimeState();

private:
   // This gets initialized from DefaultRuntimeState on Init.
   EBehaviorState _behaviorState = EBehaviorState::Interruptible;
};
