// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/ObjectMacros.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType.h"

#include "BlackboardKeyType_SingleGameplayTag.generated.h"

class UBlackboardComponent;

UCLASS(EditInlineNew, meta=(DisplayName="SingleGameplayTag"))
class OSEAI_API UBlackboardKeyType_SingleGameplayTag : public UBlackboardKeyType
{
   GENERATED_UCLASS_BODY()

   typedef FGameplayTag FDataType;
   static const FDataType InvalidValue;

   static FGameplayTag GetValue(const UBlackboardKeyType_SingleGameplayTag* keyOb, const uint8* rawData);
   static bool SetValue(UBlackboardKeyType_SingleGameplayTag* keyOb, uint8* rawData, const FGameplayTag& value);

   virtual EBlackboardCompare::Type CompareValues(const UBlackboardComponent& ownerComp, const uint8* memoryBlock, const UBlackboardKeyType* otherKeyOb, const uint8* otherMemoryBlock) const override;

protected:
   virtual FString DescribeValue(const UBlackboardComponent& ownerComp, const uint8* rawData) const override;
   virtual bool TestBasicOperation(const UBlackboardComponent& ownerComp, const uint8* memoryBlock, EBasicKeyOperation::Type op) const override;
};
