// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "OSEVoiceOverControllerComponent.h"

//ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "OSEVoiceOverEventHandlerInterface.generated.h"

//---------------------------------------------------------------------------------------
/// UOSEVoiceOverEventHandlerInterface
/// 
/// This interface provides callbacks for the OSEVoiceControllerComponent to notify
/// blueprints of when their VO has started or been interrupted.
//---------------------------------------------------------------------------------------

UINTERFACE(BlueprintType, MinimalAPI, Category = "Voice|OSE")
class UOSEVoiceOverEventHandlerInterface : public UInterface
{
   GENERATED_BODY()
};

class OSECORE_API IOSEVoiceOverEventHandlerInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintImplementableEvent, Category = "Voice|OSE")
   FGameplayTag GetVoiceIdentity() const;



   // Triggered when a request first starts to play. For conversations, this is triggered all participants.
   UFUNCTION(BlueprintImplementableEvent, Category = "Voice|OSE")
   void OnVoiceOverRequestStarted(const FOSEVoiceOverPlayingRequest& reques);

   // Trigered when a request is finished completely (not interrupted), For conversations, this is triggerd for all participants.
   UFUNCTION(BlueprintImplementableEvent, Category = "Voice|OSE")
   void OnVoiceOverRequestFinished(const FOSEVoiceOverPlayingRequest& request);

   // Trigered when a request is interrupted, For conversations, this is triggerd for all participants.
   UFUNCTION(BlueprintImplementableEvent, Category = "Voice|OSE")
   void OnVoiceOverRequestInterrupted(const FOSEVoiceOverPlayingRequest& request);

   // Triggered when a line starts to play, for conversations this happens for each node and only for the current speaker.
   UFUNCTION(BlueprintImplementableEvent, Category = "Voice|OSE")
   void OnVoiceOverLineStarted(const FOSEVoiceOverPlayingRequest& request, float startTime);

   // Triggered when a line finishes completely (not interrupted), for conversations this happens for each node and only for the current speaker.
   UFUNCTION(BlueprintImplementableEvent, Category = "Voice|OSE")
   void OnVoiceOverLineFinished(const FOSEVoiceOverPlayingRequest& request);

   // Triggered when a line is interrupted, for conversations this happens for each node and only for the curren speaker
   UFUNCTION(BlueprintImplementableEvent, Category = "Voice|OSE")
   void OnVoiceOverLineInterrupted(const FOSEVoiceOverPlayingRequest& request);

};
