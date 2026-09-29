// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "GameFramework/SafeRoom/TATSafeRoomPlayerStart.h"

// tat
#include "GameFramework/SafeRoom/TATSafeRoom.h"
#include "Player/TATPlayerState.h"

// ue5
#include "Components/CapsuleComponent.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSafeRoomPlayerStart)


ATATSafeRoomPlayerStart::ATATSafeRoomPlayerStart(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   // So the placement/visualization catches more fit stuff without having to get to map-check
   GetCapsuleComponent()->InitCapsuleSize(50.0f, 100.0f);
}

bool ATATSafeRoomPlayerStart::IsSafeRoomAvailableForInitialAssignment() const
{
   return IsValid(SafeRoom) && SafeRoom->IsAvailableForInitialAssignment();
}

void ATATSafeRoomPlayerStart::AuthorityAssignPlayerToSafeRoom(ATATPlayerState* playerState)
{
   check(IsValid(SafeRoom));
   SafeRoom->AuthoritySetOwningPlayer(playerState);
}

#if WITH_EDITOR
void ATATSafeRoomPlayerStart::CheckForErrors()
{
   Super::CheckForErrors();

   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      if (!SafeRoom)
      {
         FFormatNamedArguments arguments;
         arguments.Add(TEXT("ActorName"), FText::FromString(GetActorNameOrLabel()));
         FMessageLog("MapCheck").Warning()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::Format(FText::FromString(TEXT("{ActorName} : SafeRoom PlayerStart has no SafeRoom specified")), arguments)));
      }
   }
}
#endif
