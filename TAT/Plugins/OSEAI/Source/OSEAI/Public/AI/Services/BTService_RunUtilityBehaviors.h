// (c) 2022-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Services/BTService_RunUtilityBehaviorsBase.h"
#include "AI/Utility/UtilityAITypes.h"

#include "BTService_RunUtilityBehaviors.generated.h"

class UUtilityAIBehavior;
class UUtilityStateEvaluator;
class UUtilityBehaviorSet;

UCLASS()
class OSEAI_API UBTService_RunUtilityBehaviors : public UBTService_RunUtilityBehaviorsBase
{
   GENERATED_BODY()

public:
   UBTService_RunUtilityBehaviors(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   UFUNCTION(CallInEditor, Category = "Node|Weights")
   void RefreshWeights() { _RefreshWeights(); }

   // from UBTService
   virtual void OnInstanceCreated(UBehaviorTreeComponent& ownerComp) override;
   virtual FString GetStaticDescription() const override;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) override;
   virtual void PostLoad() override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostEditUndo() override;
#endif // WITH_EDITOR

protected:
   virtual void CreateBehaviorInstances(UUtilityAIBehaviorComponent* utilityAIBehaviorComponent);
   void CreateBehaviorInstancesFromSet(UUtilityAIBehaviorComponent* utilityAIComponent,
                                       UUtilityBehaviorSet* behaviorSet);
   void SortBehaviorInstances();

   UPROPERTY(EditAnywhere, Category = "Node")
   TArray<UUtilityBehaviorSet*> _behaviorSets;

protected:
#if WITH_EDITORONLY_DATA
   // uneditable weights, pulls from the evaluator to display inline
   UPROPERTY(Transient, EditAnywhere, meta = (EditCondition = "false"), Category = "Node|Weights")
   TArray<FString> WeightInfo;
   UPROPERTY(Transient)
   int NumBehaviors = 0;
#endif // WITH_EDITORONLY_DATA

private:
   // editor utilities
   virtual void _RefreshWeights();
};
