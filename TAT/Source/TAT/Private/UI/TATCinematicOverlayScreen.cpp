// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "UI/TATCinematicOverlayScreen.h"

// tat
#include "Online/TATGameState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCinematicOverlayScreen)

void UTATCinematicOverlayScreen::CinematicSetup(bool allowSkip)
{
   _allowSkip = allowSkip;
   _UpdateReadyCheckCounts();

   // add the screen to the viewport
   AddScreen();
}

void UTATCinematicOverlayScreen::CinematicComplete()
{
   // remove the screen from the viewport
   RemoveScreen();
}

void UTATCinematicOverlayScreen::NativeConstruct()
{
   Super::NativeConstruct();
   _UpdateReadyCheckCounts();
}

void UTATCinematicOverlayScreen::NativeDestruct()
{
   Super::NativeDestruct();
}

void UTATCinematicOverlayScreen::NativeTick(const FGeometry& myGeometry, float inDeltaTime)
{
   Super::NativeTick(myGeometry, inDeltaTime);
   _UpdateReadyCheckCounts();
}

void UTATCinematicOverlayScreen::_UpdateReadyCheckCounts()
{
   if (_allowSkip)
   {
      if (ATATGameState* gs = ATATGameState::GetTATGameState(this))
      {
         int readyToSkipCutscene = 0;
         int numPlayers = 0;
         gs->GetNumPlayersReadyForCutsceneSkip(readyToSkipCutscene, numPlayers);

         if (_numReadyToSkipCutscene != readyToSkipCutscene)
         {
            _numReadyToSkipCutscene = readyToSkipCutscene;
            OnNumReadyToSkipCutsceneChanged(_numReadyToSkipCutscene);
         }

         if (_numTotalPlayers != numPlayers)
         {
            _numTotalPlayers = numPlayers;
            OnNumTotalPlayersChanged(_numTotalPlayers);
         }
      }
   }
}

