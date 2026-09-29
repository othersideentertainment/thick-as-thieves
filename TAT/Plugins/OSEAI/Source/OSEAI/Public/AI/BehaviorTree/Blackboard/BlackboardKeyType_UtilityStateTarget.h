// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/ConsiderationInput.h"

// ue4
#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"

#include "BlackboardKeyType_UtilityStateTarget.generated.h"

class UBlackboardComponent;

UCLASS(EditInlineNew, meta=(DisplayName="UtilityStateTarget"))
class OSEAI_API UBlackboardKeyType_UtilityStateTarget : public UBlackboardKeyType
{
   GENERATED_UCLASS_BODY()

   typedef FUtilityStateTarget FDataType;
   static const FDataType InvalidValue;

   static FUtilityStateTarget GetValue(const UBlackboardKeyType_UtilityStateTarget* keyOb, const uint8* rawData);
   static bool SetValue(UBlackboardKeyType_UtilityStateTarget* keyOb, uint8* rawData, const FUtilityStateTarget& value);

   virtual EBlackboardCompare::Type CompareValues(const UBlackboardComponent& ownerComp, const uint8* memoryBlock, const UBlackboardKeyType* otherKeyOb, const uint8* otherMemoryBlock) const override;

protected:
   virtual FString DescribeValue(const UBlackboardComponent& ownerComp, const uint8* rawData) const override;
   virtual bool TestBasicOperation(const UBlackboardComponent& ownerComp, const uint8* memoryBlock, EBasicKeyOperation::Type op) const override;
};
