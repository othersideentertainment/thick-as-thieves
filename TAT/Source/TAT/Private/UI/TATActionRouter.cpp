// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATActionRouter.h"

// UE
#include <Input/CommonAnalogCursor.h>

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATActionRouter)

class FTATAnalogCursor : public FCommonAnalogCursor
{
public:
   explicit FTATAnalogCursor(const UCommonUIActionRouterBase& InActionRouter)
      : FCommonAnalogCursor(InActionRouter)
   {
   }

   virtual bool ShouldVirtualAcceptSimulateMouseButton(const FKeyEvent& InKeyEvent, EInputEvent InputEvent) const override
   {
      // Avoid simulating mouse events when in exclusive game input mode (or otherwise unspecified)
      return ActionRouter.GetActiveInputMode(ECommonInputMode::Game) != ECommonInputMode::Game;
   }
};

TSharedRef<FCommonAnalogCursor> UTATActionRouter::MakeAnalogCursor() const
{
   return FCommonAnalogCursor::CreateAnalogCursor<FTATAnalogCursor>(*this);
}
