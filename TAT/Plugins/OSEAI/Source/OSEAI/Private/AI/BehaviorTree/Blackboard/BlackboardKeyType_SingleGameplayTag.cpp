// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/BehaviorTree/Blackboard/BlackboardKeyType_SingleGameplayTag.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BlackboardKeyType_SingleGameplayTag)

const UBlackboardKeyType_SingleGameplayTag::FDataType UBlackboardKeyType_SingleGameplayTag::InvalidValue = FGameplayTag::EmptyTag;

UBlackboardKeyType_SingleGameplayTag::UBlackboardKeyType_SingleGameplayTag(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   ValueSize = sizeof(FGameplayTag);
   SupportedOp = EBlackboardKeyOperation::Basic;
}

FGameplayTag UBlackboardKeyType_SingleGameplayTag::GetValue(const UBlackboardKeyType_SingleGameplayTag* keyOb, const uint8* rawData)
{
   return GetValueFromMemory<FGameplayTag>(rawData);
}

bool UBlackboardKeyType_SingleGameplayTag::SetValue(UBlackboardKeyType_SingleGameplayTag* keyOb, uint8* rawData, const FGameplayTag& value)
{
   return SetValueInMemory<FGameplayTag>(rawData, value);
}

EBlackboardCompare::Type UBlackboardKeyType_SingleGameplayTag::CompareValues(const UBlackboardComponent& ownerComp, const uint8* memoryBlock, const UBlackboardKeyType* otherKeyOb, const uint8* otherMemoryBlock) const
{
   const FGameplayTag myValue = GetValue(this, memoryBlock);
   const FGameplayTag otherValue = GetValue((UBlackboardKeyType_SingleGameplayTag*)otherKeyOb, otherMemoryBlock);
   return myValue == otherValue ? EBlackboardCompare::Equal : EBlackboardCompare::NotEqual;
}

FString UBlackboardKeyType_SingleGameplayTag::DescribeValue(const UBlackboardComponent& ownerComp, const uint8* rawData) const
{
   return GetValue(this, rawData).ToString();
}

bool UBlackboardKeyType_SingleGameplayTag::TestBasicOperation(const UBlackboardComponent& ownerComp, const uint8* memoryBlock, EBasicKeyOperation::Type op) const
{
   const FGameplayTag tag = GetValue(this, memoryBlock);
   return (op == EBasicKeyOperation::Set) ? tag.IsValid() : !tag.IsValid();
}

