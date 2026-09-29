// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/OSEGameStateAwareActor.h"
#include "TATLoadoutCaptureMgr.generated.h"

class ATATMatchLobbyDummyPlayer;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnIsCaptureEnabledChanged, bool, bIsEnabled);

/**
 * 
 */
UCLASS()
class TAT_API ATATLoadoutCaptureMgr : public AOSEGameStateAwareActor
{
	GENERATED_BODY()

public:

   UFUNCTION(BlueprintPure, Meta = (WorldContext = "contextObject"))
   static ATATLoadoutCaptureMgr* GetLoadoutCaptureManager(const UObject* contextObject);
   UFUNCTION(BlueprintCallable)
   void EnableCapture();
   UFUNCTION(BlueprintCallable)
   void DisableCapture();

   UPROPERTY(BlueprintAssignable)
   FOnIsCaptureEnabledChanged OnIsCaptureEnabledChanged;

protected:
   void _OnGameStateFound(AGameStateBase* gameState) override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|PlayerLoadoutCapture")
   bool bIsCaptureEnabled = false;
	
private:
   UPROPERTY(EditInstanceOnly, Category = "TAT|PlayerLoadoutCapture")
   TSoftObjectPtr<ATATMatchLobbyDummyPlayer> _localPlayerDummy;
};
