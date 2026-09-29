// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Perception/StimInfo.h"

// ue4
#include "CoreMinimal.h"
#include "Perception/AIPerceptionTypes.h"

// self
#include "InterestSource.generated.h"

UENUM(BlueprintType)
enum class EInterestType : uint8
{
   Stim,
   Actor,
};

UCLASS(BlueprintType)
class OSEAI_API UInterestSource : public UObject
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintPure, Category = "AI|Alertness")
   static UInterestSource* NewInterestSourceFromStimInfo(const FStimInfo& stim, AActor* outer);

   UFUNCTION(BlueprintPure, Category = "AI|Alertness")
   static UInterestSource* NewInterestSourceFromStim(const FAIStimulus& stim, EStimType stimType, EStimSeverity stimSeverity, AActor* instigator, AActor* outer);

   UFUNCTION(BlueprintPure, Category = "AI|Alertness")
   static UInterestSource* NewInterestSourceFromActor(AActor* actor, AActor* outer);

   void InitFromStim(const FAIStimulus& stim, EStimType stimType, EStimSeverity stimSeverity, AActor* instigator);

   void InitFromActor(AActor* actor);

   UPROPERTY(BlueprintReadOnly)
   FStimInfo Stim;

   UPROPERTY(BlueprintReadOnly)
   TWeakObjectPtr<AActor> Actor;

   UPROPERTY(BlueprintReadOnly)
   EInterestType InterestType = EInterestType::Actor;
};
