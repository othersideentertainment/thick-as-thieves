// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Services/BTService_RunUtilityBehaviorsBase.h"

// ue4
#include "BehaviorTree/Services/BTService_BlackboardBase.h"

#include "BTService_RunUtilityBehavior.generated.h"

class UUtilityStateEvaluator;

USTRUCT()
struct OSEAI_API FRunUtilityBehaviorServiceItem
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere)
   FName Name;

   UPROPERTY(EditAnywhere)
   TSubclassOf<UUtilityAIBehavior> Behavior;
};

UCLASS()
class OSEAI_API UBTService_RunUtilityBehavior : public UBTService_RunUtilityBehaviorsBase
{
   GENERATED_BODY()

public:
   UBTService_RunUtilityBehavior(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   UPROPERTY(EditAnywhere, Category = "Node")
   FRunUtilityBehaviorServiceItem Behavior;

   // from UBTService
   virtual void OnInstanceCreated(UBehaviorTreeComponent& ownerComp) override;
   virtual FString GetStaticDescription() const override;

private:
   UPROPERTY(Transient)
   UUtilityStateEvaluator* _emptyEvalulator = nullptr;
};
