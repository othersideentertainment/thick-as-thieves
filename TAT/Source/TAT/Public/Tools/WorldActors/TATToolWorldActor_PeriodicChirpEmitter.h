// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/WorldActors/TATToolWorldActor_Base.h"

// ue5
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"
#include "AI/Utility/TATUtilityAITargetingGroupInterface.h"
#include "UObject/ObjectPtr.h"

#include "TATToolWorldActor_PeriodicChirpEmitter.generated.h"

class UAkAudioEvent;
class UTATPeriodicChirpEmitterToolSettings;

/// Tool World Actor that will emit visual and audio stims at its location for a duration
UCLASS()
class TAT_API ATATToolWorldActor_PeriodicChirpEmitter : public ATATToolWorldActor_Base
   , public ITATSmartObjectOwnerInterface
   , public ITATUtilityAITargetingGroupInterface
{
   GENERATED_BODY()
public:
   ATATToolWorldActor_PeriodicChirpEmitter();

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   // From ITATGearWorldActorInterface
   virtual void AuthorityDeploy_Implementation(const FTATGearWorldActorParameters& worldActorParams) override;

   /// The gameplay cue tag for the cue that should play when the world actor chirps
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio Stim", meta = (Categories="GameplayCue"))
   FGameplayTag GameplayCueTag;

   UPROPERTY(EditDefaultsOnly, Category = "Settings")
   TObjectPtr<UTATPeriodicChirpEmitterToolSettings> PeriodChirpEmitterSettings;

   /// The stim tag for when the actor chirps, for the AI hearing sense
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Audio Stim", meta = (Categories = "AI.Stim.Hearing"))
   FGameplayTag AudioStimTag;

   /// Called regularly to emit an audio stim so AI can hear the clock
   UFUNCTION(BlueprintImplementableEvent)
   void AuthorityEmitAudioStim();

   /// Called on all clients to play the audio and any other FX
   UFUNCTION(BlueprintImplementableEvent)
   void ChirpAudioAndVisuals();

   // from ITATSmartObjectOwnerInterface
   virtual UTATSmartObjectComponent* GetSmartObjectComponent() const override;
   
   // from ITATUtilityAITargetingGroupInterface
   virtual FGameplayTag GetUtilityAITargetingGroup() const override;
protected:
   // From ATATToolWorldActor_Base
   virtual bool _HasBeenActivated() const override { return true; }

   UPROPERTY(EditDefaultsOnly)
   class UTATSmartObjectComponent* _smartObjectComponent = nullptr;
   
private:
   UFUNCTION(NetMulticast, Reliable)
   void _MulticastBeginEmission(float startingServerTime, float startingDelayTime);
   UFUNCTION()
   void _OnEmitAudio();

   UPROPERTY(Transient)
   float _startingServerTime = 0.0f;

   UPROPERTY(Transient)
   float _startingDelayTime = 0.0f;
};
