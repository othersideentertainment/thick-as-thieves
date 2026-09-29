// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Developer/TATImGuiHelpers.h"

// ue
#include "Subsystems/WorldSubsystem.h"
#include "GameplayTagContainer.h"

#include "TATFireflySubsystem.generated.h"

class UTATFireflyBeaconComponent;
class ATATCharacter;

// A subsystem to register firefly beacons at runtime and manage their visibility
UCLASS()
class TAT_API UTATFireflySubsystem : public UTickableWorldSubsystem
{
   GENERATED_BODY()
   
public:
   // From USubsystem
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   // From UWorldSubsystem
   virtual void OnWorldBeginPlay(UWorld& inWorld) override;
   // From UTickableWorldSubsystem
   virtual ETickableTickType GetTickableTickType() const override;
   // From UObject
   virtual void Tick(float deltaTime) override;
   virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTATFireflySubsystem, STATGROUP_Tickables); }

   void RegisterBeacon(UTATFireflyBeaconComponent* beacon);
   void UnregisterBeacon(UTATFireflyBeaconComponent* beacon);

   TConstArrayView<TObjectPtr<UTATFireflyBeaconComponent>> GetBeacons() const { return _beacons; }
   
protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

private:
   UFUNCTION()
   void _OnThiefVisionStatusChanged(APlayerController* controller, bool thiefVisionEnabled);
   
   void _UpdateFireflyBeaconVisibility(ATATCharacter* character, bool abilityIsActive);

   UPROPERTY(Transient)
   TArray<TObjectPtr<UTATFireflyBeaconComponent>> _beacons;

   // cached thief vision state
   bool _thiefVisionEnabled = false;

   // This is true when we know for a fact that all beacons are currently hidden.
   // False indicates that at least one is visible, or the state is undetermined.
   bool _allBeaconsHidden = false;
   
#if TAT_ENABLE_DEV_TOOLS
   void _DrawDevTools(float deltaSeconds, FTATDevToolState& state);
#endif
};
