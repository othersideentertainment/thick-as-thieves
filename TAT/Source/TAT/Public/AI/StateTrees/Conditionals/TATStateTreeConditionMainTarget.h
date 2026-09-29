// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "AIController.h"
#include "StateTreeConditionBase.h"

// ose
#include "AI/Alertness/DetectionEnums.h"
#include "Math/OSEMathFunctionLibrary.h"
#include "TATStateTreeConditionMainTarget.generated.h"

USTRUCT()
struct FTATStateTreeConditionMainTargetingGroupMatchesData
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag RequiredTag;

   // If true, will test if RequiredTag matches.
   // If false, will test if RequiredTag does not match.
   UPROPERTY(EditDefaultsOnly, Category = Parameter)
   bool ShouldMatch = true;

   UPROPERTY(EditDefaultsOnly, Category = Parameter)
   bool MatchExact = true;
};

USTRUCT(meta = (DisplayName = "Check Targeting Group Matches", Category = "TAT|AI|Targeting Data"))
struct FTATStateTreeConditionMainTargetingGroupMatches : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionMainTargetingGroupMatchesData;

   FTATStateTreeConditionMainTargetingGroupMatches() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};

USTRUCT()
struct FTATStateTreeConditionMainTargetMatchesDetectionData
{
   GENERATED_BODY()
   UPROPERTY(EditDefaultsOnly, Category=Context)
   AAIController* AIController { nullptr };   

   UPROPERTY(EditDefaultsOnly, Category=Parameter)
   EActorDetectionState RequiredDetectionState { EActorDetectionState::Observing };
   
   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
};

USTRUCT(meta = (DisplayName = "Check Targeting Data Matches Detection", Category = "TAT|AI|Targeting Data"))
struct TAT_API FTATStateTreeConditionMainTargetMatchesDetection : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionMainTargetMatchesDetectionData;

   FTATStateTreeConditionMainTargetMatchesDetection() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};


USTRUCT()
struct FTATStateTreeConditionMainTargetMatchesVisibilityData
{
   GENERATED_BODY()
   UPROPERTY(EditDefaultsOnly, Category=Context)
   AAIController* AIController { nullptr };   

   UPROPERTY(EditDefaultsOnly, Category=Parameter)
   AActor* Target { nullptr };
   
   UPROPERTY(EditDefaultsOnly, Category=Parameter)
   bool RequiredVisibility { true };
};

USTRUCT(meta = (DisplayName = "Check Targeting Data Matches Visibility", Category = "TAT|AI|Targeting Data"))
struct TAT_API FTATStateTreeConditionMainTargetMatchesVisibility : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionMainTargetMatchesVisibilityData;

   FTATStateTreeConditionMainTargetMatchesVisibility() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};

USTRUCT()
struct FTATStateTreeConditionMainTargetMatchesActorData
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   AActor* Target { nullptr };

   UPROPERTY(EditDefaultsOnly, Category=Parameter)
   bool Invert { false };
};

USTRUCT(meta = (DisplayName = "Check Targeting Data Matches Specific Target", Category = "TAT|AI|Targeting Data"))
struct TAT_API FTATStateTreeConditionMainTargetMatchesActor : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionMainTargetMatchesActorData;

   FTATStateTreeConditionMainTargetMatchesActor() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};


USTRUCT()
struct FTATStateTreeConditionHasTargetInGroupData
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly, Category=Context)
   AAIController* AIController { nullptr };   

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag TargetingGroup;

   UPROPERTY(EditDefaultsOnly, Category=Parameter)
   bool Invert { false };
};

USTRUCT(meta = (DisplayName = "Check Targeting Data Exists for Targeting Group", Category = "TAT|AI|Targeting Data"))
struct TAT_API FTATStateTreeConditionHasTargetInGroup : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionHasTargetInGroupData;

   FTATStateTreeConditionHasTargetInGroup() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};

