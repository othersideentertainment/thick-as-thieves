// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATActivatableWidget.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActivatableWidget)

TOptional<FUIInputConfig> UTATActivatableWidget::GetDesiredInputConfig() const
{
   switch (InputMode)
   {
      case ETATActivatableWidgetInputMode::Default: break;

      case ETATActivatableWidgetInputMode::Menu:
      {
         return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
      }

      case ETATActivatableWidgetInputMode::Game:
      {
         return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode);
      }

      case ETATActivatableWidgetInputMode::All:
      {
         return FUIInputConfig(ECommonInputMode::All, GameMouseCaptureMode);
      }
   }

   return Super::GetDesiredInputConfig();
}

bool UTATActivatableWidget::NativeOnHandleBackAction()
{
   if(SwallowBackAction)
   {
      // returning true here so the input still is swallowed
      return true;
   }
   
   return Super::NativeOnHandleBackAction();
}
