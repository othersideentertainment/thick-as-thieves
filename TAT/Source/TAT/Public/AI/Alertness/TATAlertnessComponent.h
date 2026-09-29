// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// tat

// ose
#include "AI/Alertness/OSEAlertnessComponent.h"

// ue4
#include "CoreMinimal.h"

#include "TATAlertnessComponent.generated.h"

class UTATAlertnessSettingsAsset;

USTRUCT(BlueprintType)
struct TAT_API FTATAlertnessDecayState
{
   GENERATED_BODY()

public:
   // the server-calculated duration remaining until we decay to the next alertness level
   UPROPERTY()
   float DecayDurationRemaining = 0.0f;

   // the server-calculated total duration of this alertness level decay
   UPROPERTY()
   float TotalDecayDuration = 0.0f;

   // the server-calculated decay multiplier, replicated to clients.  we need to replicate this state because
   // clients are unaware of squads and can't look up the multiplier like the server can
   UPROPERTY()
   float DecayMultiplier = 1.0f;

   // when was all of this state last updated by the server?
   UPROPERTY()
   float ServerTimestamp = 1.0f;
};

UCLASS(BlueprintType, meta = (BlueprintSpawnableComponent))
class TAT_API UTATAlertnessComponent : public UOSEAlertnessComponent
{
   GENERATED_BODY()

public:

public:
   UTATAlertnessComponent();

   // from UActorComponent
   virtual void BeginPlay() override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   UFUNCTION(BlueprintPure, Category = "AI|Alertness")
   float GetSecondsUntilAlertnessDecay() const;

   UFUNCTION(BlueprintPure, Category = "AI|Alertness")
   float GetTotalSecondsForAlertnessDecay() const { return _decayState.TotalDecayDuration; }

   UFUNCTION(BlueprintPure, Category = "AI|Alertness")
   float GetNormalizedAlertnessDecayValue() const;

   UFUNCTION(BlueprintPure, Category = "AI|Alertness")
   float GetAlertnessDecayMultiplier() const { return _decayState.DecayMultiplier; }

   UFUNCTION(BlueprintPure, Category = "AI|Alertness")
   bool IsAlertnessCoolingDown() const;

   UFUNCTION(BlueprintPure, Category = "AI|Alertness")
   UTATAlertnessSettingsAsset* GetTATAlertnessSettingsAsset() const { return _tatAlertnessSettingsAsset; }

protected:
   // utl
   inline void _CheckHasAuthority() const { check(GetOwner()->HasAuthority()); }

   // alertness decay
   void _AuthorityTickAlertnessDecay(float deltaTime);
   void _AuthorityResetAlertLevelDecayTimer();

   // for our subclasses
   virtual bool _AuthorityDoesOwnAlertnessDecayForAlernessLevel(EAlertnessLevel alertnessLevel) const { _CheckHasAuthority(); return true; }
   virtual bool _AuthorityCanDecayAlertnessToNeutral() const { _CheckHasAuthority(); return true; }
   virtual float _AuthorityGetAlertnessDecayMultiplier() const { _CheckHasAuthority(); return 1.0f; }
   virtual float _AuthorityGetSecondsUntilAlertnessDecay() const { _CheckHasAuthority(); return _decayState.DecayDurationRemaining; }
   virtual float _AuthorityGetTotalSecondsForAlertnessDecay() const;

   // from UOSEAlertnessComponent
   virtual void _OnAlertnessLevelChangeRequested() override;
   virtual void _OnAlertnessLevelChangeRefreshed() override;
   virtual void _AuthorityBroadcastAlertLevelChanged(EAlertnessLevel oldAlertnessLevel, AActor* instigator) const override;
   
   UFUNCTION()
   void _OnRep_DecayState();

protected:
   UPROPERTY(Transient)
   UTATAlertnessSettingsAsset* _tatAlertnessSettingsAsset = nullptr;

   // the server-calculated duration remaining
   UPROPERTY(Transient, ReplicatedUsing = _OnRep_DecayState)
   FTATAlertnessDecayState _decayState;   
};
