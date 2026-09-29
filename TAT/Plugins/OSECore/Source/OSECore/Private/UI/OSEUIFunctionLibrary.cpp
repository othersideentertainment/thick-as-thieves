// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "UI/OSEUIFunctionLibrary.h"

// ose
#include "Player/OSEPlayerController.h"
#include "UI/OSEActivatableWidgetInterface.h"

// ue4
#include "CommonUIUtils.h"
#include "Components/PanelWidget.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEUIFunctionLibrary)

bool UOSEUIFunctionLibrary::GetIsMobileUIMode()
{
   return CommonUIUtils::ShouldDisplayMobileUISizes();
}

// static
UWidget* UOSEUIFunctionLibrary::GetFocusedChild(const UPanelWidget* panelWidget, APlayerController* playerController)
{
   if (IsValid(panelWidget))
   {
      for (int i = 0; i < panelWidget->GetChildrenCount(); i++)
      {
         UWidget* widget = panelWidget->GetChildAt(i);
         if (widget->HasUserFocus(playerController))
         {
            return widget;
         }
      }
   }
   
   return nullptr;
}

void UOSEUIFunctionLibrary::AnimateWidgetReversible(UUserWidget* userWidget, UWidgetAnimation* animation, EAnimResult& outAnimResult, const float startAtTime, const int32 numLoops, const float playbackSpeed, const bool restoreState, const bool forward)
{
   if (userWidget->IsAnimationPlaying(animation))
   {
      // Change direction if needed to match requested direction
      const bool shouldChangeDirection = userWidget->IsAnimationPlayingForward(animation) != forward;
      if (shouldChangeDirection)
      {
         userWidget->ReverseAnimation(animation);
      }
      
      outAnimResult = shouldChangeDirection ? EAnimResult::AnimChangedDirection : EAnimResult::AnimAlreadyPlaying;
   }
   else
   {
      // Otherwise play animation as specified
      const EUMGSequencePlayMode::Type playMode = forward ? EUMGSequencePlayMode::Forward : EUMGSequencePlayMode::Reverse;
      userWidget->PlayAnimation(animation, startAtTime, numLoops, playMode, playbackSpeed, restoreState);
      outAnimResult = EAnimResult::AnimStarted;
   }
}

// static
bool UOSEUIFunctionLibrary::SetInputMappingContext(ULocalPlayer* localPlayer, bool enabled, const FOSEInputContextPriority& inputContext)
{
   if (localPlayer == nullptr)
   {
      return false;
   }

   UEnhancedInputLocalPlayerSubsystem* localPlayerInputSubsystem = localPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
   if (localPlayerInputSubsystem == nullptr)
   {
      return false;
   }

   UInputMappingContext* inputMappingContext = inputContext.Context.LoadSynchronous();
   if (inputMappingContext == nullptr)
   {
      return false;
   }

   if (enabled)
   {
      FModifyContextOptions addContextOptions;
      addContextOptions.bIgnoreAllPressedKeysUntilRelease = false;
      addContextOptions.bForceImmediately = true;
      localPlayerInputSubsystem->AddMappingContext(inputMappingContext, static_cast<int32>(inputContext.Priority), addContextOptions);
   }
   else
   {
      FModifyContextOptions removeContextOptions;
      removeContextOptions.bIgnoreAllPressedKeysUntilRelease = true;
      removeContextOptions.bForceImmediately = true;
      localPlayerInputSubsystem->RemoveMappingContext(inputMappingContext, removeContextOptions);
   }

   FModifyContextOptions rebuildOptions;
   rebuildOptions.bIgnoreAllPressedKeysUntilRelease = false;
   rebuildOptions.bForceImmediately = true;
   localPlayerInputSubsystem->RequestRebuildControlMappings(rebuildOptions, EInputMappingRebuildType::Rebuild);
   return true;
}

// static
bool UOSEUIFunctionLibrary::BP_SetInputMappingContext(APlayerController* localPlayerController, bool enabled, const FOSEInputContextPriority& inputContext)
{
   if (localPlayerController == nullptr)
   {
      return false;
   }
   return SetInputMappingContext(localPlayerController->GetLocalPlayer(), enabled, inputContext);
}

// static
bool UOSEUIFunctionLibrary::ActivatableWidgetInterface_Activate(UWidget* widget)
{
   if (widget == nullptr || !widget->Implements<UOSEActivatableWidgetInterface>() || IOSEActivatableWidgetInterface::Execute_IsActivated(widget))
   {
      return false;
   }

   IOSEActivatableWidgetInterface::Execute_OnWidgetActivated(widget);

   // If this widget has opted in to auto-switching its input context on activation, enable that input mapping context now
   FOSEInputContextPriority inputContextPriority;
   if (IOSEActivatableWidgetInterface::Execute_UseInputContextWhenActivated(widget, inputContextPriority))
   {
      constexpr bool inputContextEnabled = true;
      if (SetInputMappingContext(widget->GetOwningLocalPlayer(), inputContextEnabled, inputContextPriority))
      {
         IOSEActivatableWidgetInterface::Execute_OnInputContextToggled(widget, inputContextEnabled);
      }
   }

   // Focus the widget if requested, allowing the widget to override what exactly should be focused
   if (IOSEActivatableWidgetInterface::Execute_ShouldFocusWidgetOnActivation(widget))
   {
      UWidget* focusTarget = IOSEActivatableWidgetInterface::Execute_GetDesiredFocusTarget(widget);
      if (focusTarget == nullptr)
      {
         focusTarget = widget;
      }
      focusTarget->SetFocus();
   }

   return true;
}

// static
bool UOSEUIFunctionLibrary::ActivatableWidgetInterface_Deactivate(UWidget* widget)
{
   if (widget == nullptr || !widget->Implements<UOSEActivatableWidgetInterface>() || !IOSEActivatableWidgetInterface::Execute_IsActivated(widget))
   {
      return false;
   }

   IOSEActivatableWidgetInterface::Execute_OnWidgetDeactivated(widget);

   // If this widget has opted in to auto-switching its input context on activation, disable that input mapping context now
   FOSEInputContextPriority inputContextPriority;
   if (IOSEActivatableWidgetInterface::Execute_UseInputContextWhenActivated(widget, inputContextPriority))
   {
      constexpr bool inputContextEnabled = false;
      if (SetInputMappingContext(widget->GetOwningLocalPlayer(), inputContextEnabled, inputContextPriority))
      {
         IOSEActivatableWidgetInterface::Execute_OnInputContextToggled(widget, inputContextEnabled);
      }
   }

   return true;
}

FEventReply UOSEUIFunctionLibrary::NavigateInDirection(FEventReply& reply, EUINavigation direction)
{
   reply.NativeReply.SetNavigation(direction, ENavigationGenesis::User);

   return reply;
}
