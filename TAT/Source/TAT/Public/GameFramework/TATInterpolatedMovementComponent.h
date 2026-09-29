// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/MovementComponent.h"
#include "TATInterpolatedMovementComponent.generated.h"

// A movement component that handles client interpolation.
UCLASS(HideCategories = (Velocity, PlanarMovement))
class TAT_API UTATInterpolatedMovementComponent : public UMovementComponent
{
	GENERATED_BODY()

public:
   UTATInterpolatedMovementComponent();


   virtual void TickComponent(float deltaTime, enum ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   virtual void SetInterpolatedComponent(USceneComponent* component);
   USceneComponent* GetInterpolatedComponent() const;
   virtual void MoveInterpolationTarget(const FVector& newLocation, const FRotator& newRotation);
   virtual void ResetInterpolation();

   UFUNCTION(BlueprintGetter, Category = Interpolation)
   bool GetInterpMovement() const { return _interpMovement; }

protected:
   void _TickInterpolation(float deltaTime);

protected:
   UPROPERTY(EditDefaultsOnly, BlueprintGetter = GetInterpMovement, Category = Interpolation)
   bool _interpMovement;

   UPROPERTY(EditDefaultsOnly, Category = Interpolation)
   bool _interpRotation;

   bool _interpolationComplete;

   UPROPERTY(EditDefaultsOnly, Category = Interpolation, meta = (ClampMin = "0"))
   float _interpLocationTime;

   UPROPERTY(EditDefaultsOnly, Category = Interpolation, meta = (ClampMin = "0"))
   float _interpRotationTime;

   UPROPERTY(EditDefaultsOnly, Category = Interpolation, meta = (ClampMin = "0"))
   float _interpLocationMaxLagDistance;

   UPROPERTY(EditDefaultsOnly, Category = Interpolation, meta = (ClampMin = "0"))
   float _interpLocationSnapToTargetDistance;
protected:

   FVector _interpLocationOffset;
   FVector _interpInitialLocationOffset;
   TWeakObjectPtr<USceneComponent> _interpolatedComponentPtr;
   FQuat _interpRotationOffset;
   FQuat _interpInitialRotationOffset;
};
