// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "BehaviorTree/Services/BTService_RunEQS.h"

#include "BTService_RunEQSIfClose.generated.h"

// Runs the EQS query if close to the target
UCLASS()
class OSEAI_API UBTService_RunEQSIfClose : public UBTService_RunEQS
{
   GENERATED_BODY()
	
public:
   UBTService_RunEQSIfClose(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());
   
   virtual void TickNode(UBehaviorTreeComponent& ownerComp, uint8* nodeMemory, float deltaSeconds) override;

protected:
   bool _IsCloseEnough(UBehaviorTreeComponent& ownerComp, const UBlackboardComponent* blackboardComponent) const;

 public:
   // distance threshold to accept as being at location
   UPROPERTY(EditAnywhere, Category = Condition, meta = (ClampMin = "0.0"))
   float CloseRadius;
};
