// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/BehaviorTree/Blackboard/BlackboardKeyType_UtilityStateTarget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BlackboardKeyType_UtilityStateTarget)

const UBlackboardKeyType_UtilityStateTarget::FDataType UBlackboardKeyType_UtilityStateTarget::InvalidValue = FUtilityStateTarget::Invalid;

UBlackboardKeyType_UtilityStateTarget::UBlackboardKeyType_UtilityStateTarget(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   ValueSize = sizeof(FUtilityStateTarget);
   SupportedOp = EBlackboardKeyOperation::Basic;
}

FUtilityStateTarget UBlackboardKeyType_UtilityStateTarget::GetValue(const UBlackboardKeyType_UtilityStateTarget* keyOb, const uint8* rawData)
{
   return GetValueFromMemory<FUtilityStateTarget>(rawData);
}

bool UBlackboardKeyType_UtilityStateTarget::SetValue(UBlackboardKeyType_UtilityStateTarget* keyOb, uint8* rawData, const FUtilityStateTarget& value)
{
   return SetValueInMemory<FUtilityStateTarget>(rawData, value);
}

EBlackboardCompare::Type UBlackboardKeyType_UtilityStateTarget::CompareValues(const UBlackboardComponent& ownerComp, const uint8* memoryBlock, const UBlackboardKeyType* otherKeyOb, const uint8* otherMemoryBlock) const
{
   const FUtilityStateTarget myValue = GetValue(this, memoryBlock);
   const FUtilityStateTarget otherValue = GetValue((UBlackboardKeyType_UtilityStateTarget*)otherKeyOb, otherMemoryBlock);
   return myValue == otherValue ? EBlackboardCompare::Equal : EBlackboardCompare::NotEqual;
}

FString UBlackboardKeyType_UtilityStateTarget::DescribeValue(const UBlackboardComponent& ownerComp, const uint8* rawData) const
{
   return GetValue(this, rawData).ToString();
}

bool UBlackboardKeyType_UtilityStateTarget::TestBasicOperation(const UBlackboardComponent& ownerComp, const uint8* memoryBlock, EBasicKeyOperation::Type op) const
{
   const FUtilityStateTarget target = GetValue(this, memoryBlock);
   return (op == EBasicKeyOperation::Set) ? target.IsValid() : !target.IsValid();
}

