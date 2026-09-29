// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2024 OtherSide Entertainment, Inc. All rights reserved.
// This file was adapted from Epic's CommonUI plugin (CommonUI/Public/CommonAnimatedSwitcher.h)
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

// ose
#include "Slate/SOSEAnimatedSwitcher.h"

// ue
#include "Components/WidgetSwitcher.h"

#include "OSEAnimatedSwitcher.generated.h"

class SOverlay;
class SSpacer;

/// A widget switcher that activates / deactivates widgets, allowing for associated animations to trigger.
UCLASS()
class OSECORE_API UOSEAnimatedSwitcher : public UWidgetSwitcher
{
   GENERATED_BODY()

public:
   // From UWidget
   virtual void ReleaseSlateResources(bool releaseChildren) override;
#if WITH_EDITOR
   virtual const FText GetPaletteCategory() override;
#endif
protected:
   virtual TSharedRef<SWidget> RebuildWidget() override;

public:
   // From UWidgetSwitcher
   virtual void SetActiveWidgetIndex(int32 index) override;
   virtual void SetActiveWidget(UWidget* widget) override;

   /// Switches to the next widget in the switcher
   /// If canWrap is true and the active widget is the last one, it will switch to the first widget
   UFUNCTION(BlueprintCallable, Category = "OSE Animated Widget Switcher")
   void ActivateNextWidget(bool canWrap);

   /// Switches to the previous widget in the switcher
   /// If canWrap is true and the active widget is the first one, it will switch to the last widget
   UFUNCTION(BlueprintCallable, Category = "OSE Animated Widget Switcher")
   void ActivatePreviousWidget(bool canWrap);

   /// Enable or disable the transition animation
   UFUNCTION(BlueprintCallable, Category = "OSE Animated Widget Switcher")
   void SetTransitionAnimationEnabled(bool newAnimationEnabled);

   /// Checks if the the transition animation is enabled
   UFUNCTION(BlueprintPure, Category = "OSE Animated Widget Switcher")
   bool IsTransitionAnimationEnabled() const { return !_instantTransition; }

   /// Is the switcher playing a transition animation?
   UFUNCTION(BlueprintCallable, Category = "OSE Animated Widget Switcher")
   bool IsTransitionPlaying() const;

   /// Checks if we're in the middle of switching the current widget index
   /// (This returns true only while SetActiveWidgetIndex_Internal is executing)
   UFUNCTION(BlueprintCallable, Category = "OSE Animated Widget Switcher")
   bool IsCurrentlySwitching() const;

protected:
   virtual void _OnWidgetActiveIndexChanged(int32 prevIndex, int32 activeIndex);
   virtual void _OnWidgetTransitionChanged(bool isTransitioning);
   virtual void _HandleOutgoingWidget(int32 outgoingWidgetIndex) {}

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FWidgetActiveIndexChangedEvent, UWidget*, prevWidget, int32, prevWidgetIndex, UWidget*, activeWidget, int32, activeWidgetIndex);
   /// Fires when the active widget displayed by the switcher changes
   UPROPERTY(BlueprintAssignable, Category = "OSE Animated Widget Switcher")
   FWidgetActiveIndexChangedEvent OnActiveWidgetIndexChanged;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTransitionChangedEvent, bool, isTransitioning);
   /// Fires when the switcher changes its transition animation state
   UPROPERTY(BlueprintAssignable, Category = "OSE Animated Widget Switcher")
   FTransitionChangedEvent OnTransitionChanged;

protected:
   /// For child widgets that implement the OSEActivatableWidgetInterface, activate and deactivate widgets when the active widget changes
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE Animated Widget Switcher")
   bool AutoActivateWidgetsOnSwitch = true;

   /// When wrapping around, don't change animation direction
   /// (eg. when advancing forward, act as if the next widget after the last one is the first one)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE Animated Widget Switcher")
   bool MaintainDirectionOnWrap = true;

   /// The type of transition to play between widgets
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE Animated Widget Switcher")
   EOSEWidgetTransitionType TransitionType = EOSEWidgetTransitionType::FadeOnly;

   /// How much to scale during transitions
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE Animated Widget Switcher", meta = (UIMin = "0.01", UIMax = "2.0", EditCondition = "TransitionType == EOSEWidgetTransitionType::Zoom", EditConditionHides))
   float ZoomScale = 0.25f;

   /// How much to scale during transitions
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE Animated Widget Switcher", meta = (UIMin = "0.0", EditCondition = "TransitionType == EOSEWidgetTransitionType::Horizontal || TransitionType == EOSEWidgetTransitionType::Vertical", EditConditionHides))
   float TranslationAmount = 200.0f;

   /// The curve function type to apply to the transition animation
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE Animated Widget Switcher")
   EOSEWidgetTransitionCurve TransitionCurveType = EOSEWidgetTransitionCurve::CubicInOut;

   /// The total duration of a single transition between widgets
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "OSE Animated Widget Switcher")
   float TransitionDuration = 0.4f;

   TSharedPtr<SOverlay> _myOverlay;
   TSharedPtr<SSpacer> _myInputGuard;
   TSharedPtr<SOSEAnimatedSwitcher> _myAnimatedSwitcher;

   /// If set, transition animations will not play
   bool _instantTransition = false;

private:
   bool _initialSetup = true;
   bool _currentlySwitching = false;

   void SetActiveWidgetIndex_Internal(int32 newIndex, EOSEWidgetAnimationDirection animationDirection = EOSEWidgetAnimationDirection::Auto);
};
