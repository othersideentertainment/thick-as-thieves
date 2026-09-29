// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/SafeRoom/TATSafeRoomConnectedActor.h"

// tat
#include "GameFramework/SafeRoom/TATSafeRoom.h"

// ue5
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSafeRoomConnectedActor)


#if WITH_EDITOR
void ATATSafeRoomConnectedActor::CheckForErrors()
{
   Super::CheckForErrors();

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      if (!_safeRoom)
      {
         FFormatNamedArguments arguments;
         arguments.Add(TEXT("ActorName"), FText::FromString(GetActorNameOrLabel()));
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} : SafeRoom-connected actor has no SafeRoom specified")), arguments)));
      }
   }
}
#endif
