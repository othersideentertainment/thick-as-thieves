// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/Tasks/AbilityTask_RM_Base.h"
#include "AbilityTask_RM_ConstrainDistance.generated.h"


UCLASS()
class OSECORE_API UAbilityTask_RM_ConstrainDistance : public UAbilityTask_RM_Base
{
   GENERATED_BODY()

public:

   /// Constructor
   UAbilityTask_RM_ConstrainDistance(const FObjectInitializer& objectInitializer);
   
   /// Apply root motion to contrain distance
   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks|Traversal", meta = (HidePin = "OwningAbility", DefaultToSelf = "OwningAbility", BlueprintInternalUseOnly = "true"))
   static UAbilityTask_RM_ConstrainDistance* ApplyRootMotionConstrainDistance(UGameplayAbility* owningAbility, const FName& taskInstanceName, const FVector& position, float distance);

protected:

   /// Derived classes implement this to allocate the root motion source
   virtual TSharedPtr<FRootMotionSource> CreateRootMotion() override;

   /// Returns the root motion source that was created with this task
   virtual TSharedPtr<FRootMotionSource> GetRootMotionSource() const override;

   /// Initializes the root motion source that was created
   virtual bool InitRootMotion(TSharedPtr<FRootMotionSource> sourcePtr) override;

   /// Ticks the root motion source. Return true to continue the task, return false to end the task
   virtual bool TickRootMotion(float DeltaTime, FRootMotionSource* sourcePtr);

protected:

   UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Position)
   FVector Position;

   UPROPERTY(BlueprintReadWrite, ReplicatedUsing = OnRep_Distance)
   float Distance;

   UFUNCTION()
   void OnRep_Position();

   UFUNCTION()
   void OnRep_Distance();
};
