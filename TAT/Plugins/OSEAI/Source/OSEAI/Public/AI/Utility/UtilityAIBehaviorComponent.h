// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/UtilityAIComponent.h"
#include "AI/Utility/UtilityAITypes.h"

// ue4
#include "CoreMinimal.h"
#include "DrawDebugHelpers.h"

// self
#include "UtilityAIBehaviorComponent.generated.h"

#if ENABLE_VISUAL_LOG
struct FVisualLogEntry;
#endif // ENABLE_VISUAL_LOG

UCLASS(Abstract)
class OSEAI_API UUtilityAIBehaviorComponent : public UUtilityAIComponent
{
   GENERATED_BODY()

public:

   UUtilityAIBehaviorComponent();
   
   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

public:
   /// Should be called by the owning AI controller when the currently executing behavior tree
   /// is finished and wants to notify the current behavior.
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility")
   void OnBehaviorTreeFinished();


#if ENABLE_VISUAL_LOG
   virtual void DescribeSelfToVisLog(FVisualLogEntry* snapshot) const override;
#endif // ENABLE_VISUAL_LOG
   AActor* GetTargetActor() const { return _TargetActor; }
protected:
   UPROPERTY(Transient)
   AActor* _TargetActor { nullptr };
   // from UUtilityAIComponent
   virtual void _OnStateEntered(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state) override;
   virtual void _OnStateExited(const FUtilityStateTarget& target, const FUtilityStateEvaluatorInstance& state) override;
};
