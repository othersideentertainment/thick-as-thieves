// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_RM_ConstrainDistance.h"
#include "Traversal/OSERootMotion.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_RM_ConstrainDistance)

typedef FOSEConstrainDistanceRootMotion TMyRootMotionSource;


// Constructor
UAbilityTask_RM_ConstrainDistance::UAbilityTask_RM_ConstrainDistance(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
   , Position(ForceInitToZero)
   , Distance(0.0f)
{

}

// Apply root motion to contrain distance
UAbilityTask_RM_ConstrainDistance* UAbilityTask_RM_ConstrainDistance::ApplyRootMotionConstrainDistance(UGameplayAbility* owningAbility, const FName& taskInstanceName, const FVector& position, float distance)
{
   UAbilityTask_RM_ConstrainDistance* myTask = NewAbilityTask<UAbilityTask_RM_ConstrainDistance>(owningAbility, taskInstanceName);
   myTask->Position = position;
   myTask->Distance = distance;
   return myTask;
}

// Derived classes implement this to allocate the root motion source
TSharedPtr<FRootMotionSource> UAbilityTask_RM_ConstrainDistance::CreateRootMotion()
{
   return MakeShared<TMyRootMotionSource>(TMyRootMotionSource());
}

// Returns the root motion source that was created with this task
TSharedPtr<FRootMotionSource> UAbilityTask_RM_ConstrainDistance::GetRootMotionSource() const
{
   auto sourcePtr = Super::GetRootMotionSource();
   if (!sourcePtr.IsValid())
      return nullptr;

   if (sourcePtr->GetScriptStruct() != TMyRootMotionSource::StaticStruct())
      return nullptr;

   return sourcePtr;
}

// Initializes the root motion source that was created
bool UAbilityTask_RM_ConstrainDistance::InitRootMotion(TSharedPtr<FRootMotionSource> sourcePtr)
{
   if (!Super::InitRootMotion(sourcePtr))
      return false;

   if (sourcePtr->GetScriptStruct() != TMyRootMotionSource::StaticStruct())
      return false;

   if (auto mySourcePtr = static_cast<TMyRootMotionSource*>(sourcePtr.Get()))
   {
      mySourcePtr->TargetLocation = Position;
      mySourcePtr->MaxDistance = Distance;
      mySourcePtr->AccumulateMode = ERootMotionAccumulateMode::Additive;
      mySourcePtr->FinishVelocityParams.Mode = ERootMotionFinishVelocityMode::ClampVelocity;
      mySourcePtr->FinishVelocityParams.ClampVelocity = 300.0f;
      return true;
   }

   return false;
}

// Ticks the root motion source. Return true to continue the task, return false to end the task
bool UAbilityTask_RM_ConstrainDistance::TickRootMotion(float deltaTime, FRootMotionSource* sourcePtr)
{
   if (!Super::TickRootMotion(deltaTime, sourcePtr))
      return false;

   if (sourcePtr == nullptr)
      return false;

   if (sourcePtr->GetScriptStruct() != TMyRootMotionSource::StaticStruct())
      return false;

   // @TODO: Need to verify if simulated ability tasks get ticked at all
   if (!IsSimulating())
   {
      if (auto mySourcePtr = static_cast<TMyRootMotionSource*>(sourcePtr))
      {
         mySourcePtr->TargetLocation = Position;
         mySourcePtr->MaxDistance = Distance;
      }
   }

   return true;
}

// Returns properties that are replicated for the lifetime of the actor channel
void UAbilityTask_RM_ConstrainDistance::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   // Autonomous and server update this independently in the ability itself
   DOREPLIFETIME_CONDITION(UAbilityTask_RM_ConstrainDistance, Position, COND_SimulatedOnly);
   DOREPLIFETIME_CONDITION(UAbilityTask_RM_ConstrainDistance, Distance, COND_SimulatedOnly);
}

void UAbilityTask_RM_ConstrainDistance::OnRep_Position()
{
   if (!IsSimulating())
      return;

   auto baseSourcePtr = GetRootMotionSource();
   if (!baseSourcePtr.IsValid())
      return;

   auto sourcePtr = StaticCastSharedPtr<TMyRootMotionSource>(baseSourcePtr);
   if (!sourcePtr.IsValid())
      return;

   sourcePtr->TargetLocation = Position;
}

void UAbilityTask_RM_ConstrainDistance::OnRep_Distance()
{
   if (!IsSimulating())
      return;

   auto baseSourcePtr = GetRootMotionSource();
   if (!baseSourcePtr.IsValid())
      return;

   auto sourcePtr = StaticCastSharedPtr<TMyRootMotionSource>(baseSourcePtr);
   if (!sourcePtr.IsValid())
      return;

   sourcePtr->MaxDistance = Distance;
}


