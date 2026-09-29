// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/AsyncTaskAnimateWidget.h"

// ue4
#include "Animation/UMGSequencePlayer.h"
#include "Animation/WidgetAnimation.h"

// ose
#include "UI/OSEUserWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AsyncTaskAnimateWidget)


// static
UAsyncTaskAnimateWidget* UAsyncTaskAnimateWidget::SpawnAsyncAnimateWidgetTask(UOSEUserWidget* userWidget, UWidgetAnimation* animation, const float startAtTime, const int32 numLoops, const float playbackSpeed, const bool restoreState, const bool forward)
{
   if (!IsValid(userWidget))
   {
      UE_LOG(LogAsyncTaskAnimateWidget, Error, TEXT("SpawnAsyncAnimateWidgetTask was passed a null userWidget!"));
      return nullptr;
   }
   if (!IsValid(animation))
   {
      UE_LOG(LogAsyncTaskAnimateWidget, Error, TEXT("SpawnAsyncAnimateWidgetTask was passed a null animation!"));
      return nullptr;
   }

   // Try to find existing async task for widget
   UAsyncTaskAnimateWidget* animateWidgetTask = userWidget->GetAsyncAnimateWidgetTask(animation);
   const bool useExistingTask = IsValid(animateWidgetTask);
   if (useExistingTask)
   {
      if (userWidget->IsAnimationPlaying(animation))
      {
         // If we want to start at a specific time, force animation interrupt
         if (startAtTime != 0.f)
         {
            UE_LOG(LogAsyncTaskAnimateWidget, Verbose, TEXT("Interrupting animation to start at time %.02f (widget = %s, animation = %s)"), startAtTime, *userWidget->GetName(), *animation->GetName());
            animateWidgetTask->_PlayAnimation(startAtTime, numLoops, playbackSpeed, restoreState, forward);
            
            // Call interrupt handler on prev node
            animateWidgetTask->Interrupted.Broadcast(forward);
         }

         // Otherwise just reverse existing animation if direction change requested
         else if (userWidget->IsAnimationPlayingForward(animation) != forward)
         {
            UE_LOG(LogAsyncTaskAnimateWidget, Verbose, TEXT("Reversed animation (widget = %s, animation = %s)"), *userWidget->GetName(), *animation->GetName());
            userWidget->ReverseAnimation(animation);

            // Call interrupt handler on prev node
            animateWidgetTask->Interrupted.Broadcast(forward);
         }
      }
      else
      {
         animateWidgetTask->_PlayAnimation(startAtTime, numLoops, playbackSpeed, restoreState, forward);
      }

      // Clear animation delegates so they only fire on this node
      animateWidgetTask->Completed.Clear();
      animateWidgetTask->Interrupted.Clear();
   }
   else
   {
      // Create new async task
      UE_LOG(LogAsyncTaskAnimateWidget, Verbose, TEXT("Spawning async animation task (widget = %s, animation = %s)"), *userWidget->GetName(), *animation->GetName());
      animateWidgetTask = NewObject<UAsyncTaskAnimateWidget>();
      animateWidgetTask->_userWidget = userWidget;
      animateWidgetTask->_animation = animation;

      animateWidgetTask->_PlayAnimation(startAtTime, numLoops, playbackSpeed, restoreState, forward);

      // Store task ref on widget for use in subsequent animation calls
      const bool success = userWidget->AddAsyncAnimateWidgetTask(animateWidgetTask, animation);
      check(success);
   }
   
   return animateWidgetTask;
}

void UAsyncTaskAnimateWidget::EndTask()
{
   UE_LOG(LogAsyncTaskAnimateWidget, Verbose, TEXT("Ending async animation task (widget = %s, animation = %s)..."), *_userWidget->GetName(), *_animation->GetName());
   if (IsValid(_userWidget))
   {
      _userWidget->RemoveAsyncAnimateWidgetTask(_animation);
   }
   if (IsValid(_player))
   {
      _player->OnSequenceFinishedPlaying().Remove(_animCallbackDelegateHandle);
   }

   SetReadyToDestroy();
   MarkAsGarbage();
}

void UAsyncTaskAnimateWidget::_OnAnimationComplete(UUMGSequencePlayer& player)
{
   UE_LOG(LogAsyncTaskAnimateWidget, Verbose, TEXT("Animation complete (widget = %s, animation = %s)"), *_userWidget->GetName(), *_animation->GetName());
   
   // Unbind when finished to avoid subsequent triggers
   // This must be called before the Completed pin is broadcast, since that can also end up calling into _PlayAnimation in this same task instance
   player.OnSequenceFinishedPlaying().Remove(_animCallbackDelegateHandle);
   _animCallbackDelegateHandle.Reset();

   Completed.Broadcast(player.IsPlayingForward());
}

void UAsyncTaskAnimateWidget::_PlayAnimation(const float startAtTime, const int32 numLoops, const float playbackSpeed, const bool restoreState, const bool forward)
{
   // Play animation
   const EUMGSequencePlayMode::Type playMode = forward ? EUMGSequencePlayMode::Forward : EUMGSequencePlayMode::Reverse;
   UUMGSequencePlayer* player = _userWidget->PlayAnimation(_animation, startAtTime, numLoops, playMode, playbackSpeed, restoreState);
   check(IsValid(player));

   // Cache player and bind to animation finished event
   _player = player;
   if (!_animCallbackDelegateHandle.IsValid())
   {
      _animCallbackDelegateHandle = player->OnSequenceFinishedPlaying().AddUObject(this, &UAsyncTaskAnimateWidget::_OnAnimationComplete);
   }
}

