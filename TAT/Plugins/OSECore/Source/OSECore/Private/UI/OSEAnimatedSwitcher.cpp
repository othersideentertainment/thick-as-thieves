// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2024 OtherSide Entertainment, Inc. All rights reserved.
// This file was adapted from Epic's CommonUI plugin (CommonUI/Private/CommonAnimatedSwitcher.cpp)
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#include "UI/OSEAnimatedSwitcher.h"

// ose
#include "UI/OSEUIFunctionLibrary.h"

// ue
#include "Components/WidgetSwitcherSlot.h"
#include "Widgets/Layout/SSpacer.h"
#include "Widgets/SOverlay.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAnimatedSwitcher)

DEFINE_LOG_CATEGORY_STATIC(LogOSEAnimatedSwitcher, Log, All);

void UOSEAnimatedSwitcher::ReleaseSlateResources(bool releaseChildren)
{
   Super::ReleaseSlateResources(releaseChildren);

   _initialSetup = true;

   _myOverlay.Reset();
   _myInputGuard.Reset();
   _myAnimatedSwitcher.Reset();
}

#if WITH_EDITOR
const FText UOSEAnimatedSwitcher::GetPaletteCategory()
{
   return FText::AsCultureInvariant(TEXT("OSE"));
}
#endif

TSharedRef<SWidget> UOSEAnimatedSwitcher::RebuildWidget()
{
   _myAnimatedSwitcher = SNew(SOSEAnimatedSwitcher)
      .InitialIndex(GetActiveWidgetIndex())
      .TransitionCurveType(TransitionCurveType)
      .TransitionDuration(TransitionDuration)
      .TransitionType(TransitionType)
      .TransitionZoomScale(ZoomScale)
      .TransitionTranslationAmount(TranslationAmount)
      .OnActiveIndexChanged_UObject(this, &UOSEAnimatedSwitcher::_OnWidgetActiveIndexChanged)
      .OnTransitionChanged_UObject(this, &UOSEAnimatedSwitcher::_OnWidgetTransitionChanged);

   // Set MyWidgetSwitcher so parent class functions work correctly
   MyWidgetSwitcher = _myAnimatedSwitcher;

   for (UPanelSlot* currentSlot : Slots)
   {
      if (UWidgetSwitcherSlot* switcherSlot = Cast<UWidgetSwitcherSlot>(currentSlot))
      {
         switcherSlot->Parent = this;
         switcherSlot->BuildSlot(MyWidgetSwitcher.ToSharedRef());
      }
   }

   return SAssignNew(_myOverlay, SOverlay)
      +SOverlay::Slot()
      [
         _myAnimatedSwitcher.ToSharedRef()
      ]
      +SOverlay::Slot()
      [
         SAssignNew(_myInputGuard, SSpacer)
         .Visibility(EVisibility::Collapsed)
      ];
}

void UOSEAnimatedSwitcher::SetActiveWidgetIndex(int32 index)
{
   SetActiveWidgetIndex_Internal(index);
}

void UOSEAnimatedSwitcher::SetActiveWidget(UWidget* widget)
{
   SetActiveWidgetIndex_Internal(GetChildIndex(widget));
}

void UOSEAnimatedSwitcher::ActivateNextWidget(bool canWrap)
{
   if (Slots.Num() <= 1)
   {
      return;
   }

   const int32 activeIndex = GetActiveWidgetIndex();
   if (activeIndex == Slots.Num() - 1)
   {
      if (canWrap)
      {
         const EOSEWidgetAnimationDirection direction = MaintainDirectionOnWrap ? EOSEWidgetAnimationDirection::Forward : EOSEWidgetAnimationDirection::Auto;
         SetActiveWidgetIndex_Internal(0, direction);
      }
   }
   else
   {
      SetActiveWidgetIndex_Internal(activeIndex + 1);
   }
}

void UOSEAnimatedSwitcher::ActivatePreviousWidget(bool canWrap)
{
   if (Slots.Num() <= 1)
   {
      return;
   }

   const int32 activeIndex = GetActiveWidgetIndex();
   if (activeIndex == 0)
   {
      if (canWrap)
      {
         const EOSEWidgetAnimationDirection direction = MaintainDirectionOnWrap ? EOSEWidgetAnimationDirection::Backward : EOSEWidgetAnimationDirection::Auto;
         SetActiveWidgetIndex_Internal(Slots.Num() - 1, direction);
      }
   }
   else
   {
      SetActiveWidgetIndex(activeIndex - 1);
   }
}

void UOSEAnimatedSwitcher::SetTransitionAnimationEnabled(bool newAnimationEnabled)
{
   _instantTransition = !newAnimationEnabled;
}

bool UOSEAnimatedSwitcher::IsTransitionPlaying() const
{
   return _myAnimatedSwitcher.IsValid() ? _myAnimatedSwitcher->IsTransitionPlaying() : false;
}

bool UOSEAnimatedSwitcher::IsCurrentlySwitching() const
{
   return _currentlySwitching;
}

void UOSEAnimatedSwitcher::_OnWidgetActiveIndexChanged(int32 prevIndex, int32 activeIndex)
{
   check(activeIndex == GetActiveWidgetIndex());

   if (AutoActivateWidgetsOnSwitch)
   {
      if (Slots.IsValidIndex(prevIndex))
      {
         UWidget* prevWidget = GetWidgetAtIndex(prevIndex);
         if (prevWidget != nullptr && prevWidget->Implements<UOSEActivatableWidgetInterface>())
         {
            UOSEUIFunctionLibrary::ActivatableWidgetInterface_Deactivate(prevWidget);
         }
      }

      if (Slots.IsValidIndex(activeIndex))
      {
         UWidget* nextWidget = GetWidgetAtIndex(activeIndex);
         if (nextWidget != nullptr && nextWidget->Implements<UOSEActivatableWidgetInterface>())
         {
            UOSEUIFunctionLibrary::ActivatableWidgetInterface_Activate(nextWidget);
         }
      }
   }

   if (Slots.IsValidIndex(activeIndex))
   {
      OnActiveWidgetIndexChanged.Broadcast(GetWidgetAtIndex(prevIndex), prevIndex, GetWidgetAtIndex(activeIndex), activeIndex);
   }
}

void UOSEAnimatedSwitcher::_OnWidgetTransitionChanged(bool isTransitioning)
{
   // While the switcher is transitioning, put up the guard to intercept all input
   _myInputGuard->SetVisibility(isTransitioning ? EVisibility::Visible : EVisibility::Collapsed);
   OnTransitionChanged.Broadcast(isTransitioning);
}

void UOSEAnimatedSwitcher::SetActiveWidgetIndex_Internal(int32 newIndex, EOSEWidgetAnimationDirection animationDirection)
{
#if WITH_EDITOR
   if (IsDesignTime())
   {
      Super::SetActiveWidgetIndex(newIndex);
      return;
   }
#endif

   TGuardValue<bool> currentlySwitchingGuard(_currentlySwitching, true);

   if (newIndex >= 0 && newIndex < Slots.Num() && (newIndex != GetActiveWidgetIndex() || _initialSetup))
   {
      const int32 prevIndex = GetActiveWidgetIndex();

      _HandleOutgoingWidget(prevIndex);

      // For now we can't call Super::SetActiveWidgetIndex since it calls MyWidgetSwitcher->SetActiveWidgetIndex(),
      // and we need to call _myAnimatedSwitcher->TransitionToIndex() instead.
      if (prevIndex != newIndex)
      {
PRAGMA_DISABLE_DEPRECATION_WARNINGS
         ActiveWidgetIndex = newIndex;
PRAGMA_ENABLE_DEPRECATION_WARNINGS
         BroadcastFieldValueChanged(FFieldNotificationClassDescriptor::ActiveWidgetIndex);
      }

      if (_myAnimatedSwitcher.IsValid())
      {
         // Ensure the index is clamped to a valid range.
         const int32 safeIndex = FMath::Clamp(newIndex, 0, FMath::Max(0, Slots.Num() - 1));
         _myAnimatedSwitcher->TransitionToIndex(safeIndex, _instantTransition, animationDirection);
      }

      // When we're setting up, and the index goes 0->0, _myAnimatedSwitcher won't fire its ActiveIndexChanged Event.
      if (_initialSetup)
      {
         _OnWidgetActiveIndexChanged(INDEX_NONE, GetActiveWidgetIndex());
      }

      _initialSetup = false;
   }
}
