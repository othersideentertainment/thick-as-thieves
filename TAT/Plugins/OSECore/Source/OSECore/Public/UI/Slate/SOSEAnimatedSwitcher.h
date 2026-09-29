// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2024 OtherSide Entertainment, Inc. All rights reserved.
// This file was adapted from Epic's CommonUI plugin (CommonUI/Public/Slate/SCommonAnimatedSwitcher.h)
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

#include "Widgets/Layout/SWidgetSwitcher.h"
#include "Animation/CurveSequence.h"

#include "SOSEAnimatedSwitcher.generated.h"

UENUM(BlueprintType)
enum class EOSEWidgetTransitionType : uint8
{
   FadeOnly       UMETA(ToolTip = "Fade transition only with no movement"),
   Horizontal     UMETA(ToolTip = "Increasing the active index goes right, decreasing goes left"),
   Vertical       UMETA(ToolTip = "Increasing the active index goes up, decreasing goes down"),
   Zoom           UMETA(ToolTip = "Increasing the active index zooms in, decreasing zooms out"),
   MAX            UMETA(Hidden)
};

UENUM(BlueprintType)
enum class EOSEWidgetTransitionCurve : uint8
{
   Linear            UMETA(ToolTip = "Linear interpolation, with no easing"),
   QuadraticIn       UMETA(ToolTip = "Quadratic ease in"),
   QuadraticOut      UMETA(ToolTip = "Quadratic ease out"),
   QuadraticInOut    UMETA(DisplayName = "Quadratic In-Out", ToolTip = "Quadratic ease in, quadratic ease out"),
   CubicIn           UMETA(ToolTip = "Cubic ease in"),
   CubicOut          UMETA(ToolTip = "Cubic ease out"),
   CubicInOut        UMETA(DisplayName = "Cubic In-Out", ToolTip = "Cubic ease in, cubic ease out"),
   MAX               UMETA(Hidden)
};

enum class EOSEWidgetAnimationDirection
{
   Auto,
   Forward,
   Backward,
};

class OSECORE_API SOSEAnimatedSwitcher : public SWidgetSwitcher
{
public:
   DECLARE_DELEGATE_TwoParams(FOnActiveIndexChanged, int32, int32);
   DECLARE_DELEGATE_OneParam(FOnTransitionChanged, bool);

   SLATE_BEGIN_ARGS(SOSEAnimatedSwitcher)
      : _InitialIndex(0)
      , _TransitionType(EOSEWidgetTransitionType::FadeOnly)
      , _TransitionCurveType(EOSEWidgetTransitionCurve::CubicInOut)
      , _TransitionDuration(0.4f)
      , _TransitionZoomScale(0.25f)
      , _TransitionTranslationAmount(200.0f)
      {
         _Visibility = EVisibility::SelfHitTestInvisible;
      }

      SLATE_ARGUMENT(int32, InitialIndex)
      SLATE_ARGUMENT(EOSEWidgetTransitionType, TransitionType)
      SLATE_ARGUMENT(EOSEWidgetTransitionCurve, TransitionCurveType)
      SLATE_ARGUMENT(float, TransitionDuration)
      SLATE_ARGUMENT(float, TransitionZoomScale)
      SLATE_ARGUMENT(float, TransitionTranslationAmount)
      SLATE_EVENT(FOnActiveIndexChanged, OnActiveIndexChanged)
      SLATE_EVENT(FOnTransitionChanged, OnTransitionChanged)

   SLATE_END_ARGS()

public:
   void Construct(const FArguments& args);

   virtual int32 OnPaint(const FPaintArgs& args, const FGeometry& allottedGeometry, const FSlateRect& cullingRect, FSlateWindowElementList& outDrawElements, int32 layerId, const FWidgetStyle& widgetStyle, bool parentEnabled) const override;
   void TransitionToIndex(int32 newWidgetIndex, bool instantTransition = false, EOSEWidgetAnimationDirection animationDirection = EOSEWidgetAnimationDirection::Auto);

   FSlot* GetChildSlot(int32 slotIndex);

   void SetTransition(float duration, EOSEWidgetTransitionCurve curve);

   bool IsTransitionPlaying() const { return _transitionSequence.IsPlaying(); }

private:
   EActiveTimerReturnType _UpdateTransition(double currentTime, float deltaTime);
   float _GetTransitionProgress() const;

private:
   float _zoomScale = 0.25f;
   float _translationAmount = 200.0f;

   /// Anim sequence for the transition; plays twice per transition
   FCurveSequence _transitionSequence;

   /// The pending active widget, set when the initial transition out completes. If set to a null value then we don't have any pending widget
   TWeakPtr<SWidget> _pendingActiveWidget;

   /// If true, we are transitioning content out and need to play the sequence again to transition it in
   bool _transitioningOut = false;

   bool _isTransitionTimerRegistered = false;
   EOSEWidgetTransitionType _transitionType = EOSEWidgetTransitionType::FadeOnly;

   FOnActiveIndexChanged _onActiveIndexChanged;
   FOnTransitionChanged _onTransitionChanged;
};
