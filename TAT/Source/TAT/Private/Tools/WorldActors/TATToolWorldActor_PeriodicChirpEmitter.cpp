// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/WorldActors/TATToolWorldActor_PeriodicChirpEmitter.h"

// tat
#include "AI/SmartObjects/TATSmartObjectComponent.h"
#include "AI/Target/TATTargetingGroups.h"
#include "Tools/ToolSettings/TATPeriodicChirpEmitterToolSettings.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolWorldActor_PeriodicChirpEmitter)

ATATToolWorldActor_PeriodicChirpEmitter::ATATToolWorldActor_PeriodicChirpEmitter()
{
   _smartObjectComponent = CreateDefaultSubobject<UTATSmartObjectComponent>(TEXT("SmartObject"));
}

#if WITH_EDITOR
EDataValidationResult ATATToolWorldActor_PeriodicChirpEmitter::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (PeriodChirpEmitterSettings == nullptr)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("TATToolWorldActor_PeriodicChirpEmitter '%s' PeriodChirpEmitterSettings is null"), *GetName())));
      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif // WITH_EDITOR

void ATATToolWorldActor_PeriodicChirpEmitter::AuthorityDeploy_Implementation(const FTATGearWorldActorParameters& worldActorParams)
{
   Super::AuthorityDeploy_Implementation(worldActorParams);

   float startingServerTime = UOSEInteractionHelpers::GetServerTimeForWrite(this);
   
   check(PeriodChirpEmitterSettings);
   // The count of delay tags is determined in BP on Authority, this way the tags can be consumed without causing a race condition among clients trying to check the tags
   int delayTagCount = worldActorParams.WorldActorData.OptionalIntValue;
   float startingDelayTime = delayTagCount * PeriodChirpEmitterSettings->StartDelayPerTagCount;

   _MulticastBeginEmission(startingServerTime, startingDelayTime);
}

UTATSmartObjectComponent* ATATToolWorldActor_PeriodicChirpEmitter::GetSmartObjectComponent() const
{
   return _smartObjectComponent;
}

FGameplayTag ATATToolWorldActor_PeriodicChirpEmitter::GetUtilityAITargetingGroup() const
{
   return TAG_AI_TargetingGroup_SmartObject_PlayerTool;
}

void ATATToolWorldActor_PeriodicChirpEmitter::_MulticastBeginEmission_Implementation(float startingServerTime, float startingDelayTime)
{
   _startingServerTime = startingServerTime;
   _startingDelayTime = startingDelayTime;
   
   if (_startingDelayTime > 0.0f)
   {
      UWorld* world = GetWorld();
      check(world);
      FTimerHandle timerHandle;
      world->GetTimerManager().SetTimer(timerHandle, this, &ATATToolWorldActor_PeriodicChirpEmitter::_OnEmitAudio, _startingDelayTime, false);
   }
   else
   {
      // If the configured delay tag is not present, just start chirping immediately
      _OnEmitAudio();
   }
}

void ATATToolWorldActor_PeriodicChirpEmitter::_OnEmitAudio()
{
   check(PeriodChirpEmitterSettings);

   // We've chirped enough, stop now
   if(UOSEInteractionHelpers::GetServerTimeForComparison(this) > _startingServerTime + PeriodChirpEmitterSettings->TotalDuration + _startingDelayTime)
   {
      Destroy();
      return;
   }

   // Shouldn't get timer callbacks if the world is going away
   UWorld* world = GetWorld();
   check(world);

   if (HasAuthority())
   {
      AuthorityEmitAudioStim();
   }

   // Don't bother with gameplay cues on dedicated servers
   if (world->GetNetMode() != NM_DedicatedServer)
   {
      ChirpAudioAndVisuals();
   }

   // Schedule the next chirp
   FTimerHandle timerHandle;
   world->GetTimerManager().SetTimer(timerHandle, this, &ATATToolWorldActor_PeriodicChirpEmitter::_OnEmitAudio, PeriodChirpEmitterSettings->ChirpFrequency, false);
}

