// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <CommonGameViewportClient.h>

#include "TATGameViewportClient.generated.h"

#define TAT_SHOW_WATERMARK (UE_BUILD_SHIPPING || UE_BUILD_TEST)

class STATWatermarkWidget;
class AOSEPlayerState;

UCLASS()
class TAT_API UTATGameViewportClient : public UCommonGameViewportClient
{
   GENERATED_BODY()

public:
   virtual void Activated(FViewport* InViewport, const FWindowActivateEvent& InActivateEvent) override;

   void ShowWatermark(bool bShow);

private:
   UFUNCTION()
   void _OnLocalPlayerStateAdded(AOSEPlayerState* _ps);

   UFUNCTION()
   void _OnGameStateSet(AGameStateBase* gameState);

   TSharedPtr<STATWatermarkWidget> _watermarkWidget;

   FString _playerName;
   FString _playerId;
};
