// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2024 OtherSide Entertainment, Inc. All rights reserved.
// This file was adapted from Epic's CommonUI plugin (CommonUI/Private/Slate/SCommonAnimatedSwitcher.cpp)
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "UI/Slate/SOSEAnimatedSwitcher.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SOSEAnimatedSwitcher)

DEFINE_LOG_CATEGORY_STATIC(LogSOSEAnimatedSwitcher, Log, All);

namespace AnimatedSwitcherHelpers
{
   ECurveEaseFunction CurveTypeToEaseFunction(EOSEWidgetTransitionCurve CurveType)
   {
      switch (CurveType)
      {
      default:
      case EOSEWidgetTransitionCurve::Linear: return ECurveEaseFunction::Linear;
      case EOSEWidgetTransitionCurve::QuadraticIn: return ECurveEaseFunction::QuadIn;
      case EOSEWidgetTransitionCurve::QuadraticOut: return ECurveEaseFunction::QuadOut;
      case EOSEWidgetTransitionCurve::QuadraticInOut: return ECurveEaseFunction::QuadInOut;
      case EOSEWidgetTransitionCurve::CubicIn: return ECurveEaseFunction::CubicIn;
      case EOSEWidgetTransitionCurve::CubicOut: return ECurveEaseFunction::CubicOut;
      case EOSEWidgetTransitionCurve::CubicInOut: return ECurveEaseFunction::CubicInOut;
      }
   }
}

void SOSEAnimatedSwitcher::Construct(const FArguments& args)
{
   SWidgetSwitcher::Construct(SWidgetSwitcher::FArguments().WidgetIndex(args._InitialIndex));

   SetCanTick(false);
   _transitioningOut = false;
   _pendingActiveWidget = nullptr;

   _transitionType = args._TransitionType;

   _zoomScale = args._TransitionZoomScale;
   _translationAmount = args._TransitionTranslationAmount;

   SetTransition(args._TransitionDuration, args._TransitionCurveType);

   _onActiveIndexChanged = args._OnActiveIndexChanged;
   _onTransitionChanged = args._OnTransitionChanged;
}

int32 SOSEAnimatedSwitcher::OnPaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& cullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& widgetStyle, bool parentEnabled) const
{
   FWidgetStyle compoundedWidgetStyle = FWidgetStyle(widgetStyle);

   // Set the alpha during transitions
   if (_transitionSequence.IsPlaying())
   {
      float alpha = _transitionSequence.GetLerp();
      if ((_transitioningOut && !_transitionSequence.IsInReverse()) || (!_transitioningOut && _transitionSequence.IsInReverse()))
      {
         alpha = 1.0f - alpha;
      }
      compoundedWidgetStyle.BlendColorAndOpacityTint(FLinearColor(1.0f, 1.0f, 1.0f, alpha));
   }

   return SWidgetSwitcher::OnPaint(args, allottedGeometry, cullingRect, outDrawElements, layerId, compoundedWidgetStyle, parentEnabled);
}

void SOSEAnimatedSwitcher::TransitionToIndex(int32 newWidgetIndex, bool instantTransition, EOSEWidgetAnimationDirection animationDirection)
{
   // Cache the widget we want to reach
   _pendingActiveWidget = GetWidget(newWidgetIndex);
   if (!_pendingActiveWidget.IsValid())
   {
      UE_LOG(LogSOSEAnimatedSwitcher, Verbose, TEXT("Called SOSEAnimatedSwitcher::TransitionToIndex('%d') to an invalid index"), newWidgetIndex);
      return;
   }

   const int32 currentIndex = GetActiveWidgetIndex();
   bool newGoalHigher = newWidgetIndex > currentIndex;
   bool newGoalLower = newWidgetIndex < currentIndex;

   const int32 prevWidgetIndex = (currentIndex != newWidgetIndex && GetTypedChildren().IsValidIndex(currentIndex)) ? currentIndex : INDEX_NONE;

   // Allow overriding the animation direction
   if (animationDirection != EOSEWidgetAnimationDirection::Auto && newWidgetIndex != currentIndex)
   {
      check(animationDirection == EOSEWidgetAnimationDirection::Forward || animationDirection == EOSEWidgetAnimationDirection::Backward);
      newGoalHigher = animationDirection == EOSEWidgetAnimationDirection::Forward;
      newGoalLower = !newGoalHigher;
   }

   if (instantTransition || _transitionSequence.GetCurve(0).DurationSeconds <= 0.f)
   {
      // Don't bother doing anything if we're already at the desired index and aren't mid-transition
      if (_transitionSequence.IsPlaying() || newGoalHigher || newGoalLower)
      {
         // Snap instantly to the target index
         _transitionSequence.JumpToEnd();
         SetActiveWidgetIndex(newWidgetIndex);

         // Technically this may not be true here, but worst-case we were transitioning away from the current index and snapped back - worth announcing
         _onActiveIndexChanged.ExecuteIfBound(prevWidgetIndex, newWidgetIndex);
      }
   }
   else if (_transitionSequence.IsPlaying())
   {
      // Already a transition in progress - see if we need to reverse it
      const bool needsReverse = (_transitionSequence.IsInReverse() && newGoalHigher)     // Currently headed to a lower index, now need to go to a higher one
                             || (!_transitionSequence.IsInReverse() && newGoalLower)     // Currently headed to a higher index, now need to go to a lower one
                             || (_transitioningOut && newWidgetIndex == currentIndex);   // Return to the index we're just now leaving
      if (needsReverse)
      {
         _transitioningOut = !_transitioningOut;
         _transitionSequence.Reverse();

         if (newWidgetIndex == currentIndex)
         {
            // Similar to above, this isn't technically true in that the true underlying active index hasn't changed, but
            //	but hitting this does mean that the requested transition target has changed. From a user perspective, this is
            //	still a scenario where one would expect to hear that the active index has changed.
            _onActiveIndexChanged.ExecuteIfBound(prevWidgetIndex, newWidgetIndex);
         }
      }
   }
   else if (newGoalHigher || newGoalLower)
   {
      SetVisibility(EVisibility::HitTestInvisible);
      _onTransitionChanged.ExecuteIfBound(true);

      if (newGoalHigher)
      {
         _transitionSequence.Play(AsShared());
      }
      else
      {
         _transitionSequence.PlayReverse(AsShared());
      }

      // If ever we're transitioning away from an empty/placeholder child index, we don't want to bother playing the outro on the current index.
      // After all, what's the point if there's nothing there? It just looks like an erroneous delay.
      TSharedPtr<SWidget> currentWidget = GetActiveWidget();
      _transitioningOut = currentWidget && currentWidget != SNullWidget::NullWidget;

      if (!_transitioningOut)
      {
         // Nothing worth animating out, so skip straight to the next index
         SetActiveWidgetIndex(newWidgetIndex);
         _onActiveIndexChanged.ExecuteIfBound(prevWidgetIndex, newWidgetIndex);
      }

      // We always want to register our timer to update the transition, even if we've skipped the outro - we still have an intro to play!
      if (!_isTransitionTimerRegistered)
      {
         _isTransitionTimerRegistered = true;
         RegisterActiveTimer(0.f, FWidgetActiveTimerDelegate::CreateSP(this, &SOSEAnimatedSwitcher::_UpdateTransition));
      }
   }
}

SWidgetSwitcher::FSlot* SOSEAnimatedSwitcher::GetChildSlot(int32 slotIndex)
{
   return GetTypedChildren().IsValidIndex(slotIndex) ? &GetTypedChildren()[slotIndex] : nullptr;
}

void SOSEAnimatedSwitcher::SetTransition(float duration, EOSEWidgetTransitionCurve curve)
{
   _transitionSequence = FCurveSequence(0.f, duration * 0.5f, AnimatedSwitcherHelpers::CurveTypeToEaseFunction(curve));
}

EActiveTimerReturnType SOSEAnimatedSwitcher::_UpdateTransition(double currentTime, float deltaTime)
{
   if (_transitionType == EOSEWidgetTransitionType::Zoom)
   {
      SetRenderTransform(FSlateRenderTransform(1 + (_zoomScale * _GetTransitionProgress())));
   }
   else if (_transitionType != EOSEWidgetTransitionType::FadeOnly)
   {
      const float offset = _translationAmount * _GetTransitionProgress();

      FVector2D translation = FVector2D::ZeroVector;
      if (_transitionType == EOSEWidgetTransitionType::Horizontal)
      {
         translation.X = -offset;
      }
      else
      {
         translation.Y = offset;
      }

      SetRenderTransform(translation);
   }

   if (!_transitionSequence.IsPlaying())
   {
      TSharedPtr<SWidget> pinnedPendingActiveWidget = _pendingActiveWidget.Pin();
      if (pinnedPendingActiveWidget.IsValid() && GetActiveWidget().Get() != pinnedPendingActiveWidget.Get())
      {
         const bool wasTransitioningOut = _transitioningOut;
         _transitioningOut = !_transitioningOut;

         if (wasTransitioningOut)
         {
            // Finished transitioning out - update the active widget and play again from the start
            const int32 prevWidgetIndex = GetActiveWidgetIndex();
            SetActiveWidget(pinnedPendingActiveWidget.ToSharedRef());
            _pendingActiveWidget = nullptr;

            // Note that any listener could decide to trigger ANOTHER transition,
            // changing the values of _pendingActiveWidget and _transitioningOut to something new.
            _onActiveIndexChanged.ExecuteIfBound(prevWidgetIndex, GetActiveWidgetIndex());
         }

         // If we are introing into or outroing from an invalid or null widget then there is no need to animate that transition.
         TSharedPtr<SWidget> currentWidget = GetActiveWidget();
         if (currentWidget && currentWidget != SNullWidget::NullWidget)
         {
            if (_transitionSequence.IsInReverse())
            {
               _transitionSequence.PlayReverse(AsShared());
            }
            else
            {
               _transitionSequence.Play(AsShared());
            }
         }
      }
      else
      {
         if (!pinnedPendingActiveWidget.IsValid())
         {
            UE_LOG(LogSOSEAnimatedSwitcher, Verbose, TEXT("SOSEAnimatedSwitcher pending widget became invalid while a transition was happening"));
         }
         _pendingActiveWidget = nullptr;
      }
   }

   // If the sequence still isn't playing, the transition is complete
   if (!_transitionSequence.IsPlaying())
   {
      SetVisibility(EVisibility::SelfHitTestInvisible);
      _onTransitionChanged.ExecuteIfBound(false);
      _isTransitionTimerRegistered = false;

      return EActiveTimerReturnType::Stop;
   }
   else
   {
      // if the sequence is playing update we need to invalidate painting so our alpha is recomputed
      Invalidate(EInvalidateWidget::Paint);
   }

   return EActiveTimerReturnType::Continue;
}

float SOSEAnimatedSwitcher::_GetTransitionProgress() const
{ 
   float progress = _transitionSequence.GetLerp();
   if ((_transitioningOut && _transitionSequence.IsInReverse()) ||
      (!_transitioningOut && _transitionSequence.IsForward()))
   {
      progress += -1.f;
   }
   return progress;
}
