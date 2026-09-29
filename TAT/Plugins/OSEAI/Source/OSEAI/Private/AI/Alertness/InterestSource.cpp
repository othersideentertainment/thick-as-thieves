// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Alertness/InterestSource.h"

// ue4
#include "Kismet/GameplayStatics.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(InterestSource)

UInterestSource* UInterestSource::NewInterestSourceFromStimInfo(const FStimInfo& stim, AActor* outer)
{
   auto* newSource = NewObject<UInterestSource>(outer);
   newSource->Stim = stim;
   newSource->InterestType = EInterestType::Stim;
   return newSource;
}

UInterestSource* UInterestSource::NewInterestSourceFromStim(const FAIStimulus& stim, EStimType stimType, EStimSeverity stimSeverity, AActor* instigator, AActor* outer)
{
   if (!instigator)
   {
      return nullptr;
   }

   auto* newSource = NewObject<UInterestSource>(outer);
   newSource->InitFromStim(stim, stimType, stimSeverity, instigator);
   return newSource;
}

UInterestSource* UInterestSource::NewInterestSourceFromActor(AActor* actor, AActor* outer)
{
   auto* newSource = NewObject<UInterestSource>(outer);
   newSource->InitFromActor(actor);
   return newSource;
}

void UInterestSource::InitFromStim(const FAIStimulus& stim, EStimType stimType, EStimSeverity stimSeverity, AActor* instigator)
{
   const float timestamp = UGameplayStatics::GetTimeSeconds(instigator);
   Stim = FStimInfo(-1, stim.StimulusLocation, instigator, stimType, stimSeverity, stim.Tag, stim.Strength, timestamp);
   Actor.Reset();
   InterestType = EInterestType::Stim;
}

void UInterestSource::InitFromActor(AActor* actor)
{
   Actor = actor;
   Stim = FStimInfo();
   InterestType = EInterestType::Actor;
}

