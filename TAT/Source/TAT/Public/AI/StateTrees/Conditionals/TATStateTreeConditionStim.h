// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreeConditionBase.h"

// ose
#include "AIController.h"
#include "AI/Perception/StimInfo.h"
#include "Character/OSETeamInterface.h"
#include "Math/OSEMathFunctionLibrary.h"
#include "TATStateTreeConditionStim.generated.h"

USTRUCT()
struct FTATStateTreeConditionStimInstanceData
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, Category="In")
   FStimInfo StimInfo;

   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   bool CheckStimTagMatches { true };
   
   UPROPERTY(EditDefaultsOnly, Category="Parameter", meta= (RowType = "/Script/TAT.TATHearingEventStimSettings"))
   FDataTableRowHandle StimInfoRow;

   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   bool CheckStimSeverityMatches { true };
   
   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   EStimSeverity TargetStimSeverity { EStimSeverity::None };
   
   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   bool CheckStimTypeMatches { true };
   
   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   EStimType StimType { EStimType::Audio };

   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   bool CheckStimInstigatorHasTags { false };

   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   FGameplayTagContainer StimInstigatorTags;    
};

USTRUCT(meta = (DisplayName = "Check Stim Event Matches", Category = "TAT|AI|Events|Stims"))
struct TAT_API FTATStateTreeConditionStimEvent : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionStimInstanceData;

   FTATStateTreeConditionStimEvent() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};


USTRUCT(meta = (DisplayName = "Check Stim Data Matches", Category = "TAT|AI|Stims"))
struct TAT_API FTATStateTreeConditionStim : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionStimInstanceData;

   FTATStateTreeConditionStim() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};

USTRUCT()
struct FTATStateTreeConditionStimInstigatorMatchesAttitudeData
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, Category="Context")
   AAIController* AIController { nullptr };
   
   UPROPERTY(EditDefaultsOnly, Category="In")
   FStimInfo StimInfo;
   
   UPROPERTY(EditDefaultsOnly, Category="Parameter")
   EOSETeamAttitude RequiredTeamAttitude { EOSETeamAttitude::Hostile };
   
   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
};

USTRUCT(meta = (DisplayName = "Check Stim Data instigator matches attitude", Category = "TAT|AI|Stims"))
struct TAT_API FTATStateTreeConditionStimInstigatorMatchesAttitude : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionStimInstigatorMatchesAttitudeData;

   FTATStateTreeConditionStimInstigatorMatchesAttitude() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};

USTRUCT()
struct FTATStateTreeConditionStimFromSharedPrivateZoneActorData
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, Category="Context")
   AAIController* AIController { nullptr };
   
   UPROPERTY(EditDefaultsOnly, Category="In")
   FStimInfo StimInfo;

   UPROPERTY(EditDefaultsOnly, Category="Parameters")
   bool Invert { false };
};


USTRUCT(meta = (DisplayName = "Check Stim Data Instigator matches private zone tags on actor", Category = "TAT|AI|Stims"))
struct TAT_API FTATStateTreeConditionStimFromSharedPrivateZoneActor : public FStateTreeConditionCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeConditionStimFromSharedPrivateZoneActorData;

   FTATStateTreeConditionStimFromSharedPrivateZoneActor() = default;
   
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual bool TestCondition(FStateTreeExecutionContext& context) const override;
};
