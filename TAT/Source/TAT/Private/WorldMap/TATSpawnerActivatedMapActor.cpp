// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "WorldMap/TATSpawnerActivatedMapActor.h"

// tat
#include "WorldMap/TATMapActorComponent.h"
#include "WorldMap/TATWorldMapSubsystem.h"

// ue
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSpawnerActivatedMapActor)

ATATSpawnerActivatedMapActor::ATATSpawnerActivatedMapActor()
{
   _mapActorComponent = CreateDefaultSubobject<UTATMapActorComponent>(TEXT("MapActorComponent"));
   _mapActorComponent->bAutoActivate = false;

   bReplicates = true;
   NetDormancy = DORM_Initial;

   // Distant players should still receive map updates
   bAlwaysRelevant = true;
}

void ATATSpawnerActivatedMapActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATSpawnerActivatedMapActor, _shouldRegisterWithMap);
}

void ATATSpawnerActivatedMapActor::AuthoritySetMapTrackingEnabled(bool enabled)
{
   check(HasAuthority());
   if (enabled != _shouldRegisterWithMap)
   {
      FlushNetDormancy();

      _shouldRegisterWithMap = enabled;
      _RefreshMapRegistration();
   }
}

void ATATSpawnerActivatedMapActor::_OnRep_ShouldRegisterWithMap()
{
   _RefreshMapRegistration();
}

void ATATSpawnerActivatedMapActor::_RefreshMapRegistration()
{
   check(IsValid(_mapActorComponent));
   _mapActorComponent->SetActive(_shouldRegisterWithMap);
}
