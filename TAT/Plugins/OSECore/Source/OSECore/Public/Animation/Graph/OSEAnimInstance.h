// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"

// OSE
#include "Animation/Graph/OSEAnimData.h"
#include "Animation/Graph/OSEAnimActorInfo.h"
#include "OSEAnimInstance.generated.h"


//--------------------------------------------------------------------------------------------------
/// Animation instance graph. This is a baseclass for optimized animation blueprints,
/// avoiding any costly blueprint thunks.
//--------------------------------------------------------------------------------------------------

UCLASS(Transient, Abstract, NotBlueprintable, BlueprintType, hideCategories = AnimInstance, meta = (BlueprintThreadSafe))
class OSECORE_API UOSEAnimInstance : public UAnimInstance
{
   GENERATED_BODY()

public:

   UOSEAnimInstance();

#if WITH_EDITORONLY_DATA 

   /// Overriding to enforce the blueprint fast path warnings all the time
   virtual bool PCV_ShouldWarnAboutNodesNotUsingFastPath() const override { return true; }

#endif // WITH_EDITORONLY_DATA

protected:

   /// Overriding to allocate our custom proxy object
   virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;

public:

   /// Overriding to allocate transient state objects
   virtual void NativeInitializeAnimation() override;

   /// Overriding to perform initialization
   virtual void NativeBeginPlay() override;

   /// Overriding to update transient state objects
   virtual void NativeUpdateAnimation(float deltaSeconds) override;

   /// Overriding to process custom IK
   virtual void NativePostEvaluateAnimation() override;

   /// Overriding to de-allocate transient state objects
   virtual void NativeUninitializeAnimation() override;

public:

   /// Returns the automatically generated actor info
   const FOSEAnimActorInfo& GetActorInfo() const { return _actorInfo; }

   /// Returns the current animation data.
   /// Use this version for property binding; otherwise read the variable directly
   UFUNCTION(BlueprintPure, Category = AnimationData)
   const FOSEAnimData& GetAnimData() const { return CurrentAnimationData; }

   /// Returns the previous frame's animation data.
   /// Use this version for property binding; otherwise read the variable directly
   UFUNCTION(BlueprintPure, Category = AnimationData)
   const FOSEAnimData& GetPreviousAnimData() const { return PreviousAnimationData; }

   /// Returns the current state flags.
   /// Use this version for property binding; otherwise read the variable directly
   UFUNCTION(BlueprintPure, Category = AnimationFlags)
   const FOSEAnimStateFlags& GetStateFlags() const { return CurrentStateFlags; }

private:

   /// This method will only be called once per frame, and will either update the data
   /// or copy it from a compatible instance. This can also be called from a linked graph
   /// or linked layer prior to copying, which ensures that the data is up-to-date.
   void UpdateOrCopyAnimationData(float deltaSeconds);

   /// Called when it's determined the animation data should be updated by this instance
   void UpdateAnimationData(float deltaSeconds);

   /// Called when it's determined the animation data should be copied from another instance
   void CopyAnimationData(const UOSEAnimInstance* srcInstance);

protected:

   /// Override this to reset / initialize data (including custom animation data).
   /// Also a convenient place to cache configuration settings.
   virtual void PerformInitialize();

   /// Override this to customize actor info building.
   /// Expected to build actor info, which is then used to update the animation data.
   /// Returns true if the info was successfully built (succeeds when able
   /// to resolve the anim instance to an actor / pawn)
   virtual bool PerformActorInfoBuild(float deltaSeconds);

   /// Override this to copy custom animation data from another instance.
   /// Expected to copy all animation data (actor info is copied earlier).
   /// Only called when we're skipping an update due to a main instance we can copy from.
   virtual void PerformDataCopy(const UOSEAnimInstance* srcInstance);

   /// Override this to update custom animation data.
   /// Expected to update the previous animation data (assign "previous=current"), then the current animation data.
   virtual void PerformDataUpdate();

   /// Override this to provide custom state change logic.
   /// Expected to compare previous and current animation data.
   /// Returns true if the state changed from previous to current.
   virtual bool PerformNewStateCheck() const;

   /// Override this to update custom last and initial animation data.
   /// Current and previous data are already updated at this point. Expected to assign "initial=current"
   /// and "last=previous" animation data.
   /// Only called when the state changed.
   virtual void PerformStateChange();

   /// Override this to perform post-update data operations.
   /// Expected to update any state that is wholly dependent on current, previous, initial, or last data.
   /// Called every update, even if actor info can't be built.
   virtual void PerformPostUpdate();

protected:

   /// The animation data for the current state
   UPROPERTY(Transient, EditInstanceOnly, BlueprintReadOnly, Category = AnimationData);
   FOSEAnimData CurrentAnimationData;

   /// The animation data from the previous update (usually the previous frame).
   /// Changes between the current and previous update are used to determine if we're in a new "state"
   UPROPERTY(Transient, EditInstanceOnly, BlueprintReadOnly, Category = AnimationData);
   FOSEAnimData PreviousAnimationData;

   /// The animation data when first transitioning to the current state
   UPROPERTY(Transient, EditInstanceOnly, BlueprintReadOnly, Category = AnimationData, AdvancedDisplay);
   FOSEAnimData InitialAnimationData;

   /// The animation data from the last state, at the moment we transitioned away from it
   UPROPERTY(Transient, EditInstanceOnly, BlueprintReadOnly, Category = AnimationData, AdvancedDisplay);
   FOSEAnimData LastAnimationData;

protected:

   /// (Automatically generated) state flags set from the current animation data
   UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = AnimationFlags);
   FOSEAnimStateFlags CurrentStateFlags;

   /// (Automatically generated) state flags set from the previous animation data
   UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = AnimationFlags, AdvancedDisplay);
   FOSEAnimStateFlags PreviousStateFlags;

   /// (Automatically generated) state flags set from the initial animation data
   UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = AnimationFlags, AdvancedDisplay);
   FOSEAnimStateFlags InitialStateFlags;

   /// (Automatically generated) state flags set from the last animation data
   UPROPERTY(Transient, VisibleInstanceOnly, BlueprintReadOnly, Category = AnimationFlags, AdvancedDisplay);
   FOSEAnimStateFlags LastStateFlags;

protected:

   /// (Automatically generated) source actor info used to populate animation state data
   UPROPERTY(Transient, VisibleInstanceOnly, Category = SourceData, AdvancedDisplay);
   FOSEAnimActorInfo _actorInfo;

private:

   /// Tracks the last frame the instance was updated
   uint64 _lastUpdateFrame = INDEX_NONE;
};
