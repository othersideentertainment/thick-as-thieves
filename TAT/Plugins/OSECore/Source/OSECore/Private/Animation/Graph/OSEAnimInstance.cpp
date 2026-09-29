// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/Graph/OSEAnimInstance.h"
#include "Animation/Graph/OSEAnimInstanceProxy.h"
#include "Animation/OSEAnimSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAnimInstance)


//--------------------------------------------------------------------------------------------------
// UOSEAnimInstance
//--------------------------------------------------------------------------------------------------

UOSEAnimInstance::UOSEAnimInstance()
   : UAnimInstance()
{ }

FAnimInstanceProxy* UOSEAnimInstance::CreateAnimInstanceProxy()
{
   return new FOSEAnimInstanceProxy(this);
}

void UOSEAnimInstance::NativeInitializeAnimation()
{
   Super::NativeInitializeAnimation();

   PerformInitialize();
}

void UOSEAnimInstance::NativeBeginPlay()
{
   Super::NativeBeginPlay();
}

void UOSEAnimInstance::NativeUpdateAnimation(float deltaSeconds)
{
   Super::NativeUpdateAnimation(deltaSeconds);

   UpdateOrCopyAnimationData(deltaSeconds);
}

void UOSEAnimInstance::NativePostEvaluateAnimation()
{
   Super::NativePostEvaluateAnimation();
}

void UOSEAnimInstance::NativeUninitializeAnimation()
{
   Super::NativeUninitializeAnimation();
}

void UOSEAnimInstance::UpdateOrCopyAnimationData(float deltaSeconds)
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAnimInstance::UpdateOrCopyAnimationData)
   // Update frame counter
   const bool bShouldUpdateOrCopy = _lastUpdateFrame != GFrameCounter;
   _lastUpdateFrame = GFrameCounter;

   if (bShouldUpdateOrCopy)
   {
      // Get the main animation instance
      const USkeletalMeshComponent* const owningComponent = GetOwningComponent();
      UOSEAnimInstance* const mainInstance = (owningComponent != nullptr) ? Cast<UOSEAnimInstance>(owningComponent->GetAnimInstance()) : nullptr;

      // We can just copy the data if we're not the main instance and it's copyable
      if ((mainInstance != nullptr) && (mainInstance != this))
      {
         // Make sure the main instance is updated first this frame
         mainInstance->UpdateOrCopyAnimationData(deltaSeconds);

         // Copy all the data from the main instance
         CopyAnimationData(mainInstance);
      }
      else
      {
         // We need to update since we're the main instance (or our main instance is not copyable)
         UpdateAnimationData(deltaSeconds);
      }
   }
}

void UOSEAnimInstance::UpdateAnimationData(float deltaSeconds)
{
   // Build actor info
   if (PerformActorInfoBuild(deltaSeconds))
   {
      // Update "current" and "previous" state data
      PerformDataUpdate();

      // Check if this is a new state
      if (PerformNewStateCheck())
      {
         // It's a new state; update "initial" and "last" state data
         PerformStateChange();
      }
   }

   // Perform any post update operations
   PerformPostUpdate();
}

void UOSEAnimInstance::CopyAnimationData(const UOSEAnimInstance* srcInstance)
{
   check(srcInstance != nullptr);
   check(srcInstance != this);

   // Copy data directly from another instance
   PerformDataCopy(srcInstance);
}


//--------------------------------------------------------------------------------------------------
// Default implementations for overrideable methods
//--------------------------------------------------------------------------------------------------

void UOSEAnimInstance::PerformInitialize()
{
   _lastUpdateFrame = INDEX_NONE;

   // Init actor info
   _actorInfo = FOSEAnimActorInfo();

   // Init animation data
   CurrentAnimationData = FOSEAnimData();
   PreviousAnimationData = FOSEAnimData();
   InitialAnimationData = FOSEAnimData();
   LastAnimationData = FOSEAnimData();

   // Init state flags
   CurrentStateFlags = FOSEAnimStateFlags();
   PreviousStateFlags = FOSEAnimStateFlags();
   InitialStateFlags = FOSEAnimStateFlags();
   LastStateFlags = FOSEAnimStateFlags();
}

bool UOSEAnimInstance::PerformActorInfoBuild(float deltaSeconds)
{
   TRACE_CPUPROFILER_EVENT_SCOPE(UOSEAnimInstance::PerformActorInfoBuild)
   const float dampingInterpolationSpeed = UOSEAnimSettings::Get().GetDampingInterpolationSpeed();
   return _actorInfo.Build(this, deltaSeconds, dampingInterpolationSpeed);
}

void UOSEAnimInstance::PerformDataCopy(const UOSEAnimInstance* srcInstance)
{
   // Copy actor info
   _actorInfo = srcInstance->_actorInfo;

   // Copy animation data
   CurrentAnimationData = srcInstance->CurrentAnimationData;
   PreviousAnimationData = srcInstance->PreviousAnimationData;
   InitialAnimationData = srcInstance->InitialAnimationData;
   LastAnimationData = srcInstance->LastAnimationData;

   // Copy state flags
   CurrentStateFlags = srcInstance->CurrentStateFlags;
   PreviousStateFlags = srcInstance->PreviousStateFlags;
   InitialStateFlags = srcInstance->InitialStateFlags;
   LastStateFlags = srcInstance->LastStateFlags;
}

void UOSEAnimInstance::PerformDataUpdate()
{
   // Save previous data and update current data
   PreviousAnimationData = CurrentAnimationData;
   CurrentAnimationData.Update(GetActorInfo());
}

bool UOSEAnimInstance::PerformNewStateCheck() const
{
   // Compare data for state changes
   if (CurrentAnimationData.State != PreviousAnimationData.State)
   {
      // High level state changed
      return true;
   }
   else
   {
      // Whether or not we consider "moving" important enough to be considered a new state
      const bool useMovingAsStateChange = UOSEAnimSettings::Get().GetUseMovingAsStateChange();
      const bool stateChangeAllowed = CurrentAnimationData.TimeElapsed >= UOSEAnimSettings::Get().GetMovingStateChangeDelay();

      if (useMovingAsStateChange && stateChangeAllowed)
      {
         const bool movingCurrently = CurrentAnimationData.Traversal.IsMoving;
         const bool movingPreviously = PreviousAnimationData.Traversal.IsMoving;

         if (movingCurrently != movingPreviously)
         {
            // Moving changed; consider this a new state
            return true;
         }
      }
   }

   // Default to no state change
   return false;
}

void UOSEAnimInstance::PerformStateChange()
{
   // New state means no time has elapsed yet. Copy elapsed time from the current
   // to the previous state, to make sure all the time is accounted for.
   PreviousAnimationData.TimeElapsed = CurrentAnimationData.TimeElapsed;
   CurrentAnimationData.TimeElapsed = 0;

   // Update the initial and last animation data
   InitialAnimationData = CurrentAnimationData;
   LastAnimationData = PreviousAnimationData;
}

void UOSEAnimInstance::PerformPostUpdate()
{
   // Always assign the state flags; they are assigned from anim data, can be used for transitions,
   // and need to reflect state set in the editor preview instance.
   CurrentStateFlags = CurrentAnimationData.State;
   PreviousStateFlags = PreviousAnimationData.State;
   InitialStateFlags = InitialAnimationData.State;
   LastStateFlags = LastAnimationData.State;

   //Update the displacement after the current and previous have been updated.
   CurrentAnimationData.Movement.Displacement.Update(CurrentAnimationData.Movement.Position.Vector - PreviousAnimationData.Movement.Position.Vector);
   CurrentAnimationData.Movement.Displacement2D.Update(CurrentAnimationData.Movement.Displacement.Vector * FVector(1.f, 1.f, 0.f));
   CurrentAnimationData.Movement.DisplacementVelocity2D.Update(CurrentAnimationData.Movement.Displacement2D.Vector * _actorInfo.OneOverDeltaT);
}

