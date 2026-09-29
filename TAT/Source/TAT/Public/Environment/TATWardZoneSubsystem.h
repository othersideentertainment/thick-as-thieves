// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Environment/TATWardZone.h"

// ue
#include "Subsystems/WorldSubsystem.h"

#include "TATWardZoneSubsystem.generated.h"

class ATATWardZone;
enum class ETATWardZonePlayerRange : uint8;

/// Manages visibility of ward zone actors for the local player
UCLASS()
class TAT_API UTATWardZoneSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:

   UTATWardZoneSubsystem();

   void RegisterWardZone(ATATWardZone* wardZone);
   void UnregisterWardZone(ATATWardZone* wardZone);

   // From UWorldSubsystem
   virtual void OnWorldBeginPlay(UWorld& inWorld) override;

protected:
   virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override { return Super::DoesSupportWorldType(worldType); }

private:
   UFUNCTION()
   void _TickWardVisualState();

   UPROPERTY(Transient)
   TObjectPtr<APlayerController> _localPlayerController;

   TMap<TWeakObjectPtr<ATATWardZone>, FTATWardZonePlayerVisibilityState> _localPlayerWardStates;

   FTimerHandle _wardVisualStateTickHandle;

   UPROPERTY(Transient)
   TSet<ATATWardZone*> _wardZones;

};
