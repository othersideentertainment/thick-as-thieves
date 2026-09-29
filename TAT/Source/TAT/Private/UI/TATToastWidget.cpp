// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATToastWidget.h"

// tat
#include "UI/TATToastSubsystem.h"

// ose
#include "Character/OSECharacterBase.h"

// ue
#include "GameFramework/PlayerController.h"
#include "Logging/LogVerbosity.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToastWidget)

DEFINE_LOG_CATEGORY_STATIC(LogTATToastWidget, Log, All);

UTATToastWidget::UTATToastWidget()
   : Super()
{
   // This callback is required for this widget to function, so default to it being enabled
   ReceiveOnLocalCharacterIsReady = true;
}

bool UTATToastWidget::RegisterForToasts()
{
   if (toastReceivedHandle.IsValid())
   {
      UE_LOG(LogTATToastWidget, Log, TEXT("Already registered"));
      return true;
   }
   auto player = GetOwningLocalPlayer();
   if (player)
   {
      if (UTATToastSubsystem* toastSubsystem = ULocalPlayer::GetSubsystem<UTATToastSubsystem>(player))
      {
         toastReceivedHandle = toastSubsystem->ToastReadyEvent.AddUObject(this, &UTATToastWidget::_OnToastReceived);
         if (toastReceivedHandle.IsValid())
         {
            UE_LOG(LogTATToastWidget, Log, TEXT("Registered to receive toast events"));
            return true;
         }
         else
         {
            UE_LOG(LogTATToastWidget, Error, TEXT("Failed to set up toast events delegate"));
         }
      }
      else
      {
         UE_LOG(LogTATToastWidget, Warning, TEXT("Couldn't get toast subsystem"));
      }
   }
   else
   {
      UE_LOG(LogTATToastWidget, Warning, TEXT("Couldn't get local player"));
   }
   return false;
}

void UTATToastWidget::_OnLocalCharacterIsReady_Implementation(AOSECharacterBase* character)
{
   Super::_OnLocalCharacterIsReady_Implementation(character);

   // If we haven't already, register to receive toast events
   if (!toastReceivedHandle.IsValid() && character != nullptr)
   {
      if (APlayerController* playerController = Cast<APlayerController>(character->GetController()))
      {
         if (ULocalPlayer* player = playerController->GetLocalPlayer())
         {
            if (UTATToastSubsystem* toastSubsystem = ULocalPlayer::GetSubsystem<UTATToastSubsystem>(player))
            {
               toastReceivedHandle = toastSubsystem->ToastReadyEvent.AddUObject(this, &UTATToastWidget::_OnToastReceived);
               if (toastReceivedHandle.IsValid())
               {
                  UE_LOG(LogTATToastWidget, Log, TEXT("Registered to receive toast events for character %s"), *GetNameSafe(character));
               }
               else
               {
                  UE_LOG(LogTATToastWidget, Error, TEXT("Failed to set up toast events delegate for character %s"), *GetNameSafe(character));
               }
            }
            else
            {
               UE_LOG(LogTATToastWidget, Warning, TEXT("Couldn't get toast subsystem from %s"), *GetNameSafe(character));
            }
         }
         else
         {
            UE_LOG(LogTATToastWidget, Warning, TEXT("Couldn't get local player from %s"), *GetNameSafe(character));
         }
      }
      else
      {
         UE_LOG(LogTATToastWidget, Warning, TEXT("Couldn't get player controller from %s"), *GetNameSafe(character));
      }
   }
}

void UTATToastWidget::_OnToastReceived(const FText& message, float duration, const TSoftObjectPtr<UPaperSprite>& icon, UAkAudioEvent* sfx)
{
   HandleOnToastReceived(message, duration, icon, sfx);
}
