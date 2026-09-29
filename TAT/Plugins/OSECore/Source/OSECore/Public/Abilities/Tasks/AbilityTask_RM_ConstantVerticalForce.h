// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_RM_Base.h"
#include "AbilityTask_RM_ConstantVerticalForce.generated.h"


DECLARE_DYNAMIC_MULTICAST_DELEGATE(FApplyRootMotionConstantVerticalForceDelegate);

struct FOSERootMotionSource_ConstantVerticalForce;

UCLASS()
class OSECORE_API UAbilityTask_RM_ConstantVerticalForce : public UAbilityTask_RM_Base
{
   GENERATED_BODY()

public:
   using TMyRootMotionSource = FOSERootMotionSource_ConstantVerticalForce;

   /// Constructor
   UAbilityTask_RM_ConstantVerticalForce(const FObjectInitializer& objectInitializer);


   UPROPERTY(BlueprintAssignable)
   FApplyRootMotionConstantVerticalForceDelegate OnFinish;
   
   // It has a constant vertical velocity, but allows maintaining some horizontal with configurable damping and air control coefficients
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "TRUE"))
   static UAbilityTask_RM_ConstantVerticalForce* ApplyRootMotionConstantVerticalForce
   (
      UGameplayAbility* OwningAbility,
      FName TaskInstanceName,
      float Strength,
      float Duration,
      float LateralDamping,
      float LateralMovementCoefficient,
      UCurveFloat* StrengthOverTime,
      ERootMotionFinishVelocityMode VelocityOnFinishMode,
      FVector SetVelocityOnFinish,
      float ClampVelocityOnFinish
   );

protected:

   /// Derived classes implement this to allocate the root motion source
   virtual TSharedPtr<FRootMotionSource> CreateRootMotion() override;

   /// Returns the root motion source that was created with this task
   virtual TSharedPtr<FRootMotionSource> GetRootMotionSource() const override;

   /// Initializes the root motion source that was created
   virtual bool InitRootMotion(TSharedPtr<FRootMotionSource> sourcePtr) override;

   virtual void OnTimedOut() override;

protected:

   UPROPERTY(Replicated)
   float VerticalForce;

   UPROPERTY(Replicated)
   float LateralDamping;

   UPROPERTY(Replicated)
   float LateralMovementCoefficient;

   UPROPERTY(Replicated)
   UCurveFloat* StrengthOverTime;

   UPROPERTY(Replicated)
   ERootMotionFinishVelocityMode FinishVelocityMode;

   UPROPERTY(Replicated)
   FVector FinishSetVelocity;

   UPROPERTY(Replicated)
   float FinishClampVelocity;
};
