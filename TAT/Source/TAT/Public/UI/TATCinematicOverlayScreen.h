// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "TATScreenWidget.h"

#include "TATCinematicOverlayScreen.generated.h"

class ATATMultiplayerLevelSequenceActor;

UCLASS()
class TAT_API UTATCinematicOverlayScreen : public UTATScreenWidget
{
   GENERATED_BODY()

public:
   void CinematicSetup(bool allowSkip);
   void CinematicComplete();

   // from UUserWidget
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;
   virtual void NativeTick(const FGeometry& myGeometry, float inDeltaTime) override;

   UFUNCTION(BlueprintPure)
   bool IsSkipAllowed() const { return _allowSkip; }
   UFUNCTION(BlueprintPure)
   int GetNumReadyToSkipCutscene() const { return _numReadyToSkipCutscene; }
   UFUNCTION(BlueprintPure)
   int GetNumTotalPlayers() const { return _numTotalPlayers; }

   UFUNCTION(BlueprintImplementableEvent)
   void OnNumReadyToSkipCutsceneChanged(int numReadyChecked);
   UFUNCTION(BlueprintImplementableEvent)
   void OnNumTotalPlayersChanged(int numTotalPlayers);

private:
   void _UpdateReadyCheckCounts();

private:
   bool _allowSkip = false;
   int _numReadyToSkipCutscene = 0;
   int _numTotalPlayers = 0;
};
