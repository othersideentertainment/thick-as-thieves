// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "UI/TATScreenWidget.h"

// tat
#include "Player/TATPlayerController.h"
#include "UI/TATScreenMetadata.h"
#include "UI/TATScreenMgr.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Character/OSECharacterBase.h"

// ue
#include "AbilitySystemComponent.h"
#include "Misc/DataValidation.h"
#include "UObject/WeakObjectPtr.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATScreenWidget)

void UTATScreenWidget::NativeConstruct()
{
   Super::NativeConstruct();
}

void UTATScreenWidget::NativeDestruct()
{
   Super::NativeDestruct();
}

void UTATScreenWidget::NativeOnInitialized()
{
   _initialVisibility = GetVisibility();
   Super::NativeOnInitialized();
}

TSharedRef<SWidget> UTATScreenWidget::RebuildWidget()
{
   TSharedRef<SWidget> widget = Super::RebuildWidget();

   // Add SWidget metadata with references to self and parent screen (if stack not empty).
   // This is necessary to allow TATNavigationConfig::OnNavigationChangedFocus() to recognize SWidgets associated with TATScreenWidgets
   // so we can ensure input/focus doesn't touch widgets below the top screen.
   if (widget->GetMetaData<FTATScreenMetadata>() == nullptr)
   {
      TWeakObjectPtr<UTATScreenWidget> me = MakeWeakObjectPtr<UTATScreenWidget>(this);
      widget->AddMetadata<FTATScreenMetadata>(MakeShared<FTATScreenMetadata>(me));
   }

   return widget;
}

#if WITH_EDITOR
EDataValidationResult UTATScreenWidget::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   // added screens should be focusable (avoid infinite focus recursion crash)
   if (!IsFocusable())
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("'%s' is a screen widget, but is not focusable (fix class defaults 'Is Focusable')"), *GetName())));
   }

   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif // WITH_EDITOR

void UTATScreenWidget::HandleOnScreenAddedToStack(ATATPlayerController& ownerPC)
{
   // add to the viewport
   AddToViewport();

   // for bp
   _OnScreenAddedToViewport();
}

void UTATScreenWidget::HandleOnScreenRemovedFromStack(ATATPlayerController& ownerPC)
{
   // remove from viewport
   RemoveFromParent();

   // for bp
   _OnScreenRemovedFromViewport();
}

void UTATScreenWidget::HandleOnScreenOnTopOfStack(ATATPlayerController& ownerPC)
{
   // this screen is on top of the stack -- set any input locking state we need to
   _SetInputState(ownerPC, true);

   // Restore visibility to initial value if needed
   if (GetVisibility() != _initialVisibility)
   {
      SetVisibility(_initialVisibility);
   }

   // for bp
   _OnScreenOnTopOfStack();
}

void UTATScreenWidget::HandleOnScreenRemovedFromTopOfStack(ATATPlayerController& ownerPC)
{
   // this screen is being removed from the top of the stack, set/reset any state that we need to
   _SetInputState(ownerPC, false);

   // Cache screen visibility and set self (and children) as non hit-testable to prevent focus/navigation from reaching our widgets
   _initialVisibility = GetVisibility();
   SetVisibility(ESlateVisibility::HitTestInvisible);

   // for bp
   _OnScreenRemovedFromTopOfStack();
}

void UTATScreenWidget::HandleOnTopOfStackWhenFocusRefreshed()
{
   _InitializeFocus();
}

void UTATScreenWidget::OnScreenReceiveFocus(TSharedPtr<SWidget> focusWidget, uint32 focusUser)
{
   check(focusWidget.IsValid());
   _lastFocusedWidget = focusWidget;
   _lastFocusedUser = focusUser;

   UE_LOG(LogTATScreenMgr, VeryVerbose, TEXT("%s - Caching widget of type %s"), *GetName(), *focusWidget->GetTypeAsString());
}

void UTATScreenWidget::AddScreen()
{
   UTATScreenMgr::AddScreenToViewport(this, this);
}

void UTATScreenWidget::RemoveScreen()
{
   UTATScreenMgr::RemoveScreenFromViewport(this, this);
}

bool UTATScreenWidget::IsInScreenStack() const
{
   if (UTATScreenMgr* mgr = UTATScreenMgr::TryGetScreenManager(this))
   {
      return mgr->IsScreenInStack(this);
   }
   return false;
}

void UTATScreenWidget::_OnScreenAddedToViewport_Implementation()
{
   // bp stub
}

void UTATScreenWidget::_OnScreenRemovedFromViewport_Implementation()
{
   // bp stub
}

void UTATScreenWidget::_OnScreenOnTopOfStack_Implementation()
{
   // bp stub
}

void UTATScreenWidget::_OnScreenRemovedFromTopOfStack_Implementation()
{
   // bp stub
}

void UTATScreenWidget::_TryRemoveScreenFromMgr()
{
   if (UTATScreenMgr* screenMgr = UTATScreenMgr::TryGetScreenManager(this))
   {
      if (screenMgr->IsScreenInStack(this))
      {
         screenMgr->RemoveScreen(this);
      }
   }
}

void UTATScreenWidget::_SetInputState(ATATPlayerController& ownerPC, bool isOnTop)
{
   if (IgnoreMoveInput)
      ownerPC.SetIgnoreMoveInput(isOnTop);
   if (IgnoreLookInput)
      ownerPC.SetIgnoreLookInput(isOnTop);

   if (DisableAbilitySystem)
   {
      ownerPC.SetIgnoreAbilityInput(isOnTop);
   }

   if (isOnTop)
   {
      _ReassignFocus();
   }
}

void UTATScreenWidget::_ReassignFocus()
{
   // Prefer focusing a widget that previously had focus (usually means another screen appeared above this one in the stack, taking its focus)
   if (_lastFocusedWidget.IsValid())
   {
      UE_LOG(LogTATScreenMgr, Verbose, TEXT("%s - Restoring focus to widget of type %s"), *GetName(), *_lastFocusedWidget.Pin()->GetTypeAsString());
      FSlateApplication::Get().SetUserFocus(_lastFocusedUser, _lastFocusedWidget.Pin(), EFocusCause::SetDirectly);
   }
   else
   {
      UE_LOG(LogTATScreenMgr, Verbose, TEXT("%s - Falling back to _InitializeFocus"), *GetName());
      _InitializeFocus();
   }
}

void UTATScreenWidget::_InitializeFocus_Implementation()
{
   SetUserFocus(GetOwningPlayer());
}

void UTATScreenWidget::SetIgnoreAbilityInput(ATATPlayerController* ownerPC, bool shouldIgnore)
{
   //WARNING - this was added to fix a specific bug with the pause menu and should be removed when no longer required.
   // See the definition for more details as well as BPUI_ManagementScreen and BPUI_StartMenuOptions
   if (IsValid(ownerPC))
   {
      ownerPC->SetIgnoreAbilityInput(shouldIgnore);
   }
}

