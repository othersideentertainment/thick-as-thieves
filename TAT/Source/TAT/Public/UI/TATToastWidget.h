// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "UI/TATUserWidget.h"

// ue
#include "Paper2D/Classes/PaperSprite.h"

#include "TATToastWidget.generated.h"

class UPaperSprite;
class UAkAudioEvent;

UCLASS(meta = (DisableNativeTick))
class TAT_API UTATToastWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   UTATToastWidget();

   /// Event when a toast should be displayed
   /// N.B. If the toast is on cooldown, or another toast is currently displayed when it's requested, this event will not fire
   /// This event only triggers when we should actually display the toast
   UFUNCTION(BlueprintImplementableEvent)
   void HandleOnToastReceived(const FText& message, float duration, const TSoftObjectPtr<UPaperSprite>& icon, UAkAudioEvent* sfx);
   
   UFUNCTION(BlueprintCallable)
   bool RegisterForToasts();

protected:
   void _OnLocalCharacterIsReady_Implementation(AOSECharacterBase* character) override;

private:
   void _OnToastReceived(const FText& message, float duration, const TSoftObjectPtr<UPaperSprite>& icon, UAkAudioEvent* sfx);

   FDelegateHandle toastReceivedHandle;
};
