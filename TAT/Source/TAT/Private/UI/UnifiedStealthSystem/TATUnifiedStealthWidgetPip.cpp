// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/UnifiedStealthSystem/TATUnifiedStealthWidgetPip.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUnifiedStealthWidgetPip)

void UTATUnifiedStealthWidgetPip::_HandleStateChange_Implementation(bool isActivated)
{
}

void UTATUnifiedStealthWidgetPip::SetState(const bool state)
{
   if(_IsCurrentlyActive.IsSet() == false)
   {
      _IsCurrentlyActive = state;
      _HandleStateChange(_IsCurrentlyActive.GetValue());
   }
   else
   {
      if(_IsCurrentlyActive != state)
      {
         _IsCurrentlyActive = state;
         _HandleStateChange(_IsCurrentlyActive.GetValue());
      }
   }
}
