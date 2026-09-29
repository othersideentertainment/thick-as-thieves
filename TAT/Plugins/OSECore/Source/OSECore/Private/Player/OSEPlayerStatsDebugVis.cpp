// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Player/OSEPlayerStatsDebugVis.h"

// ose
#include "Player/OSEPlayerState.h"

// ue5
#include "Engine/Canvas.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
namespace PlayerStatsDebugVis
{
   void OnShowDebugInfo(AHUD* hud, UCanvas* canvas, const FDebugDisplayInfo& displayInfo, float& yl, float& yPos)
   {
      static const FName kNamePlayerStats("PlayerStats");
      if (canvas == nullptr || !hud->ShouldDisplayDebug(kNamePlayerStats) || hud->PlayerOwner == nullptr)
      {
         return;
      }

      const AOSEPlayerState* playerState = hud->PlayerOwner->GetPlayerState<AOSEPlayerState>();
      if(playerState == nullptr)
      {
         return;
      }

      const FOSEPlayerStats& stats = playerState->GetPlayerStats();
      FDisplayDebugManager& displayDebugManager = canvas->DisplayDebugManager;
      displayDebugManager.SetDrawColor(FColor::Purple);

      displayDebugManager.DrawString(TEXT("PLAYER STATS"));
      for(const FOSEPlayerStat& stat : stats.Stats)
      {
         displayDebugManager.DrawString(FString::Printf(TEXT("%s: %d"), *stat.Tag.ToString(), stat.IntValue));
      }
   }
}
#endif
