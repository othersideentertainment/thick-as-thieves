// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "GameFramework/EscapeRoutes/TATEscapeRouteWorldSubsystem.h"

// ue
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEscapeRouteWorldSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogTATEscapeRouteWorldSubsystem, Log, All);

const TSet<ATATEscapePoint*>& UTATEscapeRouteWorldSubsystem::GetAllCurrentlyValidEscapePoints() const
{
   return _registeredEscapePoints;
}

void UTATEscapeRouteWorldSubsystem::RegisterEscapePoint(ATATEscapePoint* escapePoint)
{
   bool alreadyPresent = false;
   _registeredEscapePoints.Add(escapePoint, &alreadyPresent);
   if (!alreadyPresent)
   {
      OnEscapeRouteAdded.Broadcast(escapePoint);
   }
   else
   {
      UE_LOG(LogTATEscapeRouteWorldSubsystem, Warning, TEXT("RegisterEscapePoint() called with previously-registered escape point %s!"), *escapePoint->GetName());
   }
}

void UTATEscapeRouteWorldSubsystem::UnregisterEscapePoint(ATATEscapePoint* escapePoint)
{
   if (_registeredEscapePoints.Remove(escapePoint) > 0)
   {
      OnEscapeRouteRemoved.Broadcast(escapePoint);
   }
   else
   {
      UE_LOG(LogTATEscapeRouteWorldSubsystem, Warning, TEXT("UnregisterEscapePoint() called with unregistered escape point %s!"), *escapePoint->GetName());
   }
}

void UTATEscapeRouteWorldSubsystem::AuthorityBroadcastEscapeRouteUsed(ATATEscapePoint* escapePoint, APlayerState* player)
{
   if (!IsValid(escapePoint))
   {
      UE_LOG(LogTATEscapeRouteWorldSubsystem, Error, TEXT("AuthorityBroadcastEscapeRouteUsed() called with invalid escapePoint!"));
      return;
   }
   if (!IsValid(player))
   {
      UE_LOG(LogTATEscapeRouteWorldSubsystem, Error, TEXT("AuthorityBroadcastEscapeRouteUsed() called with invalid player!"));
      return;
   }

   check(escapePoint->HasAuthority());
   AuthorityOnEscapeRouteUsed.Broadcast(escapePoint, player);
}
