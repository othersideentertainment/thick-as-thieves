// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATButton.h"

// TAT
#include "UI/TATUIFunctionLibrary.h"

// UE
#include <CommonActionWidget.h>
#include <CommonTextBlock.h>
#include <Input/UIActionBinding.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATButton)

void UTATButton::NativePreConstruct()
{
   Super::NativePreConstruct();

   if (ButtonTextWidget)
   {
      ButtonTextWidget->SetText(ButtonText);
   }
}

void UTATButton::NativeDestruct()
{
   Super::NativeDestruct();

   if (bRepresentsActionBinding)
   {
      UnbindRepresentedActionEvents();

      bRepresentsActionBinding = false;
      RepresentedActionBindingHandle = {};
   }
}

void UTATButton::NativeOnCurrentTextStyleChanged()
{
   Super::NativeOnCurrentTextStyleChanged();

   if (ButtonTextWidget)
   {
      ButtonTextWidget->SetStyle(GetCurrentTextStyleClass());
   }
}

void UTATButton::NativeOnHovered()
{
   Super::NativeOnHovered();

   if (HoveredSoundEvent)
   {
      UTATUIFunctionLibrary::PlaySound(HoveredSoundEvent);
   }

   if (HoveredAnim)
   {
      PlayAnimationForward(HoveredAnim);
   }
}

void UTATButton::NativeOnUnhovered()
{
   Super::NativeOnUnhovered();

   if (HoveredAnim)
   {
      PlayAnimationReverse(HoveredAnim);
   }
}

void UTATButton::NativeOnSelected(bool bBroadcast)
{
   Super::NativeOnSelected(bBroadcast);

   if (SelectedSoundEvent)
   {
      UTATUIFunctionLibrary::PlaySound(SelectedSoundEvent);
   }

   if (SelectedAnim)
   {
      PlayAnimationForward(SelectedAnim);
   }
}

void UTATButton::NativeOnDeselected(bool bBroadcast)
{
   Super::NativeOnDeselected(bBroadcast);

   if (SelectedAnim)
   {
      PlayAnimationReverse(SelectedAnim);
   }
}

void UTATButton::NativeOnPressed()
{
   Super::NativeOnPressed();

   if (PressedSoundEvent)
   {
      UTATUIFunctionLibrary::PlaySound(PressedSoundEvent);
   }

   if (PressedAnim)
   {
      PlayAnimationForward(PressedAnim);
   }
}

void UTATButton::NativeOnReleased()
{
   Super::NativeOnReleased();

   if (PressedAnim)
   {
      PlayAnimationReverse(PressedAnim);
   }
}

void UTATButton::NativeOnClicked()
{
   Super::NativeOnClicked();

   if (bRepresentsActionBinding)
   {
      RepresentedActionExecuteEvent.ExecuteIfBound();
   }
}

void UTATButton::UpdateInputActionWidget()
{
   if (bRepresentsActionBinding)
   {
      if (InputActionWidget)
      {
         InputActionWidget->SetInputActionBinding(RepresentedActionBindingHandle);
         UpdateInputActionWidgetVisibility();
      }

      if (ButtonTextWidget)
      {
         ButtonTextWidget->SetText(RepresentedActionBindingHandle.GetDisplayName());
      }
   }
   else
   {
      Super::UpdateInputActionWidget();
   }
}

void UTATButton::SetRepresentedAction(FUIActionBindingHandle bindingHandle)
{
   UnbindRepresentedActionEvents();

   bRepresentsActionBinding = true;
   RepresentedActionBindingHandle = bindingHandle;

   BindRepresentedActionEvents();
   UpdateInputActionWidget();
}

void UTATButton::BindRepresentedActionEvents()
{
   const TSharedPtr<FUIActionBinding> actionBinding = FUIActionBinding::FindBinding(RepresentedActionBindingHandle);
   if (actionBinding.IsValid())
   {
      RepresentedActionExecuteEvent = actionBinding->OnExecuteAction;

      actionBinding->OnHoldActionProgressed.AddUObject(this, &ThisClass::NativeOnActionProgress);
      actionBinding->OnHoldActionPressed.AddUObject(this, &ThisClass::NativeOnPressed);
      actionBinding->OnHoldActionReleased.AddUObject(this, &ThisClass::NativeOnReleased);
   }
}

void UTATButton::UnbindRepresentedActionEvents()
{
   const TSharedPtr<FUIActionBinding> actionBinding = FUIActionBinding::FindBinding(RepresentedActionBindingHandle);
   if (actionBinding.IsValid())
   {
      actionBinding->OnHoldActionProgressed.RemoveAll(this);
      actionBinding->OnHoldActionPressed.RemoveAll(this);
      actionBinding->OnHoldActionReleased.RemoveAll(this);
   }

   RepresentedActionExecuteEvent = {};
}

void UTATButton::SetRepresentedTab_Implementation(UTATTabList* tabList, const FTATTabDescriptor& tabDescriptor)
{
   if (ButtonTextWidget)
   {
      ButtonTextWidget->SetText(tabDescriptor.DisplayName);
   }
}

void UTATButton::SetButtonText(const FText& text)
{
   ButtonText = text;

   if (ButtonTextWidget)
   {
      ButtonTextWidget->SetText(ButtonText);
   }
}
