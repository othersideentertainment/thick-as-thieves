// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Services/BTService_RunUtilityBehaviors.h"
#include "AI/Utility/ConsiderationInput.h"
#include "AI/Utility/UtilityAITypes.h"

// ue4

#include "BTService_RunGoalTargetedUtilityBehaviors.generated.h"

class UUtilityAIComponent;

UCLASS()
class OSEAI_API UBTService_RunGoalTargetedUtilityBehaviors : public UBTService_RunUtilityBehaviors
{
   GENERATED_BODY()

public:
   UBTService_RunGoalTargetedUtilityBehaviors(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   // from UBTService
   virtual void OnSearchStart(FBehaviorTreeSearchData& searchData) override;
   virtual void TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds) override;
   virtual void OnCeaseRelevant(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory) override;
   virtual FString GetStaticDescription() const override;

private:
   void _UpdateTarget(UBehaviorTreeComponent& ownerComp);
   void _AddTarget(UBehaviorTreeComponent& ownerComp, EUtilityStateTargeting type, const FUtilityStateTarget& target);
   void _ClearTargets(UBehaviorTreeComponent& ownerComp);
};
