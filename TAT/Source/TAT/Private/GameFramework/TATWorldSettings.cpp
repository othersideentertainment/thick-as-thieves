// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/TATWorldSettings.h"

// ue
#include "GameplayTagContainer.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWorldSettings)

void ATATWorldSettings::NotifyBeginPlay()
{
   Super::NotifyBeginPlay();

   OnWorldBeginPlay.Broadcast();
}

void ATATWorldSettings::BeginPlay()
{
   Super::BeginPlay();

   const UWorld* world = GetWorld();
   if (HasAuthority() && IsValid(world) && world->GetWorldSettings() != this)
   {
      // Skip replicating world settings objects for anything but the main world
      SetReplicates(false);
   }
}

/* static */
ATATWorldSettings& ATATWorldSettings::Get(const UObject* contextObj)
{
   check(contextObj && contextObj->GetWorld());
   return *CastChecked<ATATWorldSettings>(contextObj->GetWorld()->GetWorldSettings());
}

 /* static */
 ATATWorldSettings* ATATWorldSettings::GetTATWorldSettings(const UObject* contextObj)
 {
    return &Get(contextObj);
 }
