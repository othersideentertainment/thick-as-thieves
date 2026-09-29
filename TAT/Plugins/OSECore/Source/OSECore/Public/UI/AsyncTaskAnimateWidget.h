// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Kismet/BlueprintAsyncActionBase.h"

#include "AsyncTaskAnimateWidget.generated.h"

class UUMGSequencePlayer;
class UWidgetAnimation;
class UOSEUserWidget;

DEFINE_LOG_CATEGORY_STATIC(LogAsyncTaskAnimateWidget, Log, All);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAnimationTaskDelegate, bool, outForward);

/**
 * Blueprint node to assist in playing bi-directional animations on widgets with support for changing direction mid-animation.
 */
UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class OSECORE_API UAsyncTaskAnimateWidget : public UBlueprintAsyncActionBase
{
   GENERATED_BODY()
   
public:
   // Called when the animation plays to completion
   UPROPERTY(BlueprintAssignable)
   FAnimationTaskDelegate Completed;

   // Called when the animation is reversed mid-animation
   UPROPERTY(BlueprintAssignable)
   FAnimationTaskDelegate Interrupted;

public:
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", DefaultToSelf = "userWidget"), DisplayName = "SpawnAnimateWidgetTaskAsync", Category = "User Interface|OSE")
   static UAsyncTaskAnimateWidget* SpawnAsyncAnimateWidgetTask(UOSEUserWidget* userWidget, UWidgetAnimation* animation, const float startAtTime = 0.0f, const int32 numLoops = 1, const float playbackSpeed = 1.0f, const bool restoreState = false, const bool forward = true);

   const UWidgetAnimation* GetAnimation() const { return _animation; }
   
   // Should be called on spawned task instances once they are no longer needed. Otherwise, the task will persist until RemoveFromParent() is called on the associated widget.
   UFUNCTION(BlueprintCallable)
   void EndTask();

private:
   void _OnAnimationComplete(UUMGSequencePlayer& player);

   void _PlayAnimation(const float startAtTime, const int32 numLoops, const float playbackSpeed, const bool restoreState, const bool forward);

private:
   UPROPERTY(Transient)
   UOSEUserWidget* _userWidget;

   UPROPERTY(Transient)
   UWidgetAnimation* _animation;

   UPROPERTY(Transient)
   UUMGSequencePlayer* _player;

   // Handle bound to FOnSequenceFinishedPlaying event of UUMGSequencePlayer playing the animation
   FDelegateHandle _animCallbackDelegateHandle;
};
