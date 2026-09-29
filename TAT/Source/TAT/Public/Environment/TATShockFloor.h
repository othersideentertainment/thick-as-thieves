// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "TATOnOffSequence.h"

// ose
#include "Abilities/OSEActorsWithAppliedEffectsSet.h"
#include "Interactables/OSEInteractableToggle.h"

// ue5
#include "Perception/AISightTargetInterface.h"

#include "TATShockFloor.generated.h"

class UTATElectricalDeviceComponent;
class UTATOnOffSequence;
enum class ETATOnOffState : uint8;


// This could be given a more generic name, but
UCLASS()
class TAT_API ATATShockFloor
   : public AOSESyncedToggle
   , public IAISightTargetInterface
{
	GENERATED_BODY()
	
public:
	// Sets default values for this actor's properties
	ATATShockFloor();

   // from IAISightTargetInterface
   virtual bool CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor = nullptr,
      const bool* wasVisible = nullptr, int32* userData = nullptr) const override;

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

public:
   UFUNCTION(BlueprintPure)
   ETATOnOffState GetState() const { return _floorState; }

   UFUNCTION(BlueprintPure)
   bool IsInState(ETATOnOffState possibleState) const { return GetState() == possibleState; }

   virtual void NotifyActorBeginOverlap(AActor* otherActor) override;
   virtual void NotifyActorEndOverlap(AActor* otherActor) override;

protected:
   virtual void _OnStateChanged(bool bIsOn, bool bWasRecent) override;
   void _CancelTimer();

   /// The state has changed, but it may have changed a long time ago
   UFUNCTION(BlueprintImplementableEvent, DisplayName = "OnFloorStateChanged", meta = (BlueprintProtected, ScriptName = "OnStateChanged"))
   void BP_OnFloorStateChanged(ETATOnOffState newState, ETATOnOffState previousState, bool wasRecent);

protected:
   bool _IsValidTarget(const AActor* actor) const;
   
   void _UpdateOverlapCollision(bool enabled);
   
   UFUNCTION()
   void _OnPoweredChanged(bool isPowered);
   
   void _SampleSequence(bool wasRecent);
   
   void _SetFloorState(ETATOnOffState newState, bool wasRecent);
   const TATOnOffSequence::FCompiledSequence& _GetCompiledSequence() const;

   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, meta = (AllowPrivateAccess = "True"))
   TObjectPtr<UTATElectricalDeviceComponent> _electricalDeviceComponent;

   // time in seconds that the floor is charging before it fires
   UPROPERTY(EditDefaultsOnly, Category = Durations)
   float _chargingDuration;

   // On/off sequence to play
   // On indefinitely if not specified
   UPROPERTY(EditAnywhere, Category = Durations)
   TObjectPtr<UTATOnOffSequence> _sequence;

   UPROPERTY(EditDefaultsOnly, Category = Target)
   FGameplayTagContainer _requiredTargetTags;
   
   UPROPERTY(EditDefaultsOnly, Category = Target)
   FGameplayTagContainer _blockedTargetTags;

   // Effect to apply to targets while in active shock floor
   UPROPERTY(EditDefaultsOnly, Category = Target)
   TSubclassOf<UGameplayEffect> _shockedEffect;

   UPROPERTY(Transient)
   FOSEActorsWithAppliedEffectsSet _appliedEffects;
   
   ETATOnOffState _floorState;
   FTimerHandle _nextStateTimer;
};
