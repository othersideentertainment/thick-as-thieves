// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/OSEHUD.h"

// ose
#include "Player/OSEPlayerController.h"
#include "Player/AimAssist/OSEAimAssistComponent.h"

// ue4
#include "Engine/Canvas.h"
#include "Engine/Texture2D.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEHUD)

AOSEHUD::AOSEHUD()
   : Super()
{

}

void AOSEHUD::DrawHUD()
{
   Super::DrawHUD();

#if !UE_BUILD_SHIPPING
   AOSEPlayerController* pc = AOSEPlayerController::GetLocalOSEPlayerController(this);
   UOSEAimAssistComponent* aimAssist = pc ? pc->GetAimAssist() : nullptr;
   if (aimAssist)
   {
      aimAssist->DebugDrawHUD(*this);
   }
#endif
}

