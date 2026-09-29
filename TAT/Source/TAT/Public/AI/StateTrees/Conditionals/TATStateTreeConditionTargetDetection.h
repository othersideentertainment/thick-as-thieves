// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreeConditionBase.h"

// tat
#include "AI/Alertness/DetectionEnums.h"

// ose
#include "Character/OSETeamInterface.h"
#include "Math/OSEMathFunctionLibrary.h"
#include "TATStateTreeConditionTargetDetection.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeConditionTargetDetectionData
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly, Category=Context)
   AAIController* AIController { nullptr };   

   UPROPERTY(EditDefaultsOnly, Category=Input)
   AActor* Target { nullptr };
   
   UPROPERTY(EditDefaultsOnly, Category=Parameter)
   EActorDetectionState RequiredDetectionState { EActorDetectionState::Observing };
   
   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
};

USTRUCT(meta = (DisplayName = "Check Target Knowledge with Detection", Category = "TAT|AI|Target"))
struct TAT_API FTATStateTreeConditionTargetDetection : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionTargetDetectionData;

   FTATStateTreeConditionTargetDetection() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};

USTRUCT()
struct FTATStateTreeConditionTargetWithAttitudeData
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly, Category=Context)
   AAIController* AIController { nullptr };   

   UPROPERTY(EditDefaultsOnly, Category=Input)
   AActor* Target { nullptr };
   
   UPROPERTY(EditDefaultsOnly, Category=Parameter)
   EOSETeamAttitude RequiredTeamAttitude { EOSETeamAttitude::Hostile };
   
   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
};

USTRUCT(meta = (DisplayName = "Check Target Knowledge with Attitude", Category = "TAT|AI|Target"))
struct TAT_API FTATStateTreeConditionTargetWithAttitude : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionTargetWithAttitudeData;

   FTATStateTreeConditionTargetWithAttitude() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};
