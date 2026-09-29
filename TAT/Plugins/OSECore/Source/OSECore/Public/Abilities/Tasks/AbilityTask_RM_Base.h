// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/OSEAbilityTask.h"
#include "GameFramework/RootMotionSource.h"
#include "AbilityTask_RM_Base.generated.h"


/// Base class for root motion ability tasks
UCLASS(Abstract)
class OSECORE_API UAbilityTask_RM_Base : public UOSEAbilityTask
{
   GENERATED_BODY()
   
public:

   /// Constructor
   UAbilityTask_RM_Base(const FObjectInitializer& objectInitializer);

   /// Called right before being marked for destruction due to network replication
   virtual void PreDestroyFromReplication() override;

   /// Tick function for this task, if bTickingTask == true
   virtual void TickTask(float deltaTime) override;

protected:

   /// Called to trigger the actual task once the delegates have been set up
   virtual void Activate() override;

   /// Initializes the task on simulated proxies
   virtual void InitSimulatedTask(UGameplayTasksComponent& inGameplayTasksComponent) override;

   /// End and CleanUp the task - may be called by the task itself or by the task owner if the owner is ending.
   virtual void OnDestroy(bool inOwnerFinished) override;

protected:

   /// Derived classes implement this to allocate the root motion source
   virtual TSharedPtr<FRootMotionSource> CreateRootMotion();

   /// Returns the root motion source that was created with this task
   virtual TSharedPtr<FRootMotionSource> GetRootMotionSource() const;

   /// Initializes the root motion source that was created
   virtual bool InitRootMotion(TSharedPtr<FRootMotionSource> SourcePtr);

   /// Creates, initializes, and applies the root motion source
   virtual bool InitAndApplyRootMotion(const FGameplayAbilityActorInfo* actorInfo);

   /// Ticks the root motion source. Return true to continue the task, return false to end the task
   virtual bool TickRootMotion(float deltaTime, FRootMotionSource* sourcePtr);

   /// Returns true if the root motion has timed out or is finished
   virtual bool HasTimedOut() const;

   /// Returns true if the root motion has timed out or is finished
   virtual void OnTimedOut() {}

protected:

   /// Cached character for the avatar actor
   UPROPERTY(Transient)
   class ACharacter* _rootMotionCharacter;

   /// Cached character movemement component
   UPROPERTY(Transient)
   class UCharacterMovementComponent* _charMovementComp;

   /// Root motion source ID
   UPROPERTY(Transient)
   uint16 _rootMotionSourceID;

protected:

   UPROPERTY(Replicated)
   uint16 _priority;

   UPROPERTY(Replicated)
   float _duration;
};
