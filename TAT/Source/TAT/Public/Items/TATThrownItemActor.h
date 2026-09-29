// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Items/TATItemActor.h"
#include "TATThrownItemActor.generated.h"

class UProjectileMovementComponent;

/// An item actor class that is directly thrown, and may be caught
///
/// This does not preclude item-tools from spawning projectile versions of themselves
/// that are separate from their normal in-world version (as that is simpler)
UCLASS()
class TAT_API ATATThrownItemActor : public ATATItemActor
{
   GENERATED_BODY()

public:
   ATATThrownItemActor();

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityStartThrowing(const FVector& velocity);

   virtual void Tick(float deltaTime) override;

protected:

   virtual void BeginPlay() override;

   virtual void PostNetReceiveLocationAndRotation() override;
   virtual void PostNetReceiveVelocity(const FVector& newVelocity) override;
   virtual void OnRep_ReplicateMovement() override;

   virtual void _OnTakenAuthority(ACharacter* inTakingCharacter) override;

   UFUNCTION(BlueprintImplementableEvent, meta=(DisplayName=OnAuthorityThrowStart))
   void BP_OnAuthorityThrowStart();
   UFUNCTION(BlueprintImplementableEvent, meta = (DisplayName = OnInFlightChanged))
   void BP_OnInFlightChanged(bool inFlight);

   UFUNCTION()
   void _OnProjectileStop(const FHitResult& impactResult);

   UFUNCTION()
   void OnRep_InFlight(bool oldValue);
   void _OnInFlightChanged();
   void _AuthoritySetInFlight(bool newInFlight);

protected:
   /// Projectile movement component
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Projectile", meta = (AllowPrivateAccess = "true"))
   UProjectileMovementComponent* _projectileMovement = nullptr;

   UPROPERTY(Transient, ReplicatedUsing=OnRep_InFlight)
   bool _inFlight;

   // Fake rotation to a fixed rotation after a throw
   UPROPERTY(EditDefaultsOnly, Category = "Projectile|Rotation")
   bool _rotateDuringThrow;
   UPROPERTY(EditDefaultsOnly, Category = "Projectile|Rotation", meta= (EditCondition= _rotateDuringThrow))
   FRotator _desiredRelativeRotation;
   FRotator _targetRotation;
   UPROPERTY(EditDefaultsOnly, Category = "Projectile|Rotation", meta = (EditCondition = _rotateDuringThrow))
   float _throwRotationRate;
};
