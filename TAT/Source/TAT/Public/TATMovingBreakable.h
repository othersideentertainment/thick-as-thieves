// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Breakables/TATBreakableBase.h"

// ue
#include "CoreMinimal.h"

#include "TATMovingBreakable.generated.h"

class UTATInterpolatedMovementComponent;

/// An actor that has Server-authoritative movement and can take damage (e.g. Pickpocket Fairy).
/// This class uses the TATInterpolatedMovementComponent to interpolate it's visual root on the client in order to make client movement replication smooth.
UCLASS()
class TAT_API ATATMovingBreakable : public ATATBreakableBase
{
   GENERATED_BODY()

public:
   ATATMovingBreakable();

   virtual void BeginPlay() override;

protected:
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<USceneComponent> _movementRoot = nullptr;
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<USceneComponent> _visualRoot = nullptr;
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UTATInterpolatedMovementComponent> _movementComponent = nullptr;

   // from AActor
   virtual void PostNetReceiveLocationAndRotation() override;
};
