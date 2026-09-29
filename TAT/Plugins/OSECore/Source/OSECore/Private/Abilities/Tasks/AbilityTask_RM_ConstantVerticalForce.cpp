// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Tasks/AbilityTask_RM_ConstantVerticalForce.h"
#include "Traversal/OSERootMotion.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_RM_ConstantVerticalForce)


UAbilityTask_RM_ConstantVerticalForce::UAbilityTask_RM_ConstantVerticalForce(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

UAbilityTask_RM_ConstantVerticalForce* UAbilityTask_RM_ConstantVerticalForce::ApplyRootMotionConstantVerticalForce(UGameplayAbility* OwningAbility, FName TaskInstanceName, float Strength, float Duration, float LateralDamping, float LateralMovementCoefficient, UCurveFloat* StrengthOverTime, ERootMotionFinishVelocityMode VelocityOnFinishMode, FVector SetVelocityOnFinish, float ClampVelocityOnFinish)
{
   UAbilityTask_RM_ConstantVerticalForce* MyTask = NewAbilityTask<UAbilityTask_RM_ConstantVerticalForce>(OwningAbility, TaskInstanceName);

   MyTask->VerticalForce = Strength;
   MyTask->LateralDamping = LateralDamping;
   MyTask->LateralMovementCoefficient = LateralMovementCoefficient;
   MyTask->_duration = Duration;
   MyTask->StrengthOverTime = StrengthOverTime;
   MyTask->FinishVelocityMode = VelocityOnFinishMode;
   MyTask->FinishSetVelocity = SetVelocityOnFinish;
   MyTask->FinishClampVelocity = ClampVelocityOnFinish;

   return MyTask;
}

TSharedPtr<FRootMotionSource> UAbilityTask_RM_ConstantVerticalForce::CreateRootMotion()
{
   return MakeShared<TMyRootMotionSource>(TMyRootMotionSource());
}

// Returns the root motion source that was created with this task
TSharedPtr<FRootMotionSource> UAbilityTask_RM_ConstantVerticalForce::GetRootMotionSource() const
{
   auto sourcePtr = Super::GetRootMotionSource();
   if (!sourcePtr.IsValid())
      return nullptr;

   if (sourcePtr->GetScriptStruct() != TMyRootMotionSource::StaticStruct())
      return nullptr;

   return sourcePtr;
}

// Initializes the root motion source that was created
bool UAbilityTask_RM_ConstantVerticalForce::InitRootMotion(TSharedPtr<FRootMotionSource> sourcePtr)
{
   if (!Super::InitRootMotion(sourcePtr))
      return false;

   if (sourcePtr->GetScriptStruct() != TMyRootMotionSource::StaticStruct())
      return false;

   if (auto mySourcePtr = static_cast<TMyRootMotionSource*>(sourcePtr.Get()))
   {
      mySourcePtr->VerticalForce = VerticalForce;
      mySourcePtr->StrengthOverTime = StrengthOverTime;
      mySourcePtr->LateralDamping = LateralDamping;
      mySourcePtr->LateralMovementCoefficient = LateralMovementCoefficient;
      mySourcePtr->AccumulateMode = ERootMotionAccumulateMode::Override;
      mySourcePtr->FinishVelocityParams.Mode = FinishVelocityMode;
      mySourcePtr->FinishVelocityParams.SetVelocity = FinishSetVelocity;
      mySourcePtr->FinishVelocityParams.ClampVelocity = FinishClampVelocity;
      return true;
   }

   return false;
}

void UAbilityTask_RM_ConstantVerticalForce::OnTimedOut()
{
   AActor* actor = GetAvatarActor();
   if (actor)
   {
      actor->ForceNetUpdate();
      if (ShouldBroadcastAbilityTaskDelegates())
      {
         OnFinish.Broadcast();
      }
   }
}

// Returns properties that are replicated for the lifetime of the actor channel
void UAbilityTask_RM_ConstantVerticalForce::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(UAbilityTask_RM_ConstantVerticalForce, VerticalForce);
   DOREPLIFETIME(UAbilityTask_RM_ConstantVerticalForce, LateralDamping);
   DOREPLIFETIME(UAbilityTask_RM_ConstantVerticalForce, LateralMovementCoefficient);
   DOREPLIFETIME(UAbilityTask_RM_ConstantVerticalForce, StrengthOverTime);
   DOREPLIFETIME(UAbilityTask_RM_ConstantVerticalForce, FinishVelocityMode);
   DOREPLIFETIME(UAbilityTask_RM_ConstantVerticalForce, FinishSetVelocity);
   DOREPLIFETIME(UAbilityTask_RM_ConstantVerticalForce, FinishClampVelocity);
}


