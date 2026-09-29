// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/TATPlayerStart.h"

// tat
#include "Developer/TATProjectSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerStart)

ATATPlayerStart::ATATPlayerStart(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{   

}

void ATATPlayerStart::PostInitializeComponents()
{
   Super::PostInitializeComponents();
}

void ATATPlayerStart::BeginPlay()
{
   Super::BeginPlay();
}

FVector ATATPlayerStart::GetRespawnMarkerLocation() const
{
   if(IsValid(RespawnMarkerLocationProxy))
   {
      return RespawnMarkerLocationProxy->GetActorLocation();
   }
   return GetActorLocation() + FVector(0, 0, UTATProjectSettings::Get().PlayerStartRespawnMarkerZOffset);
}

