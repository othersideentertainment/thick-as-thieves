// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Environment/TATAreaVolume.h"

// tat
#include "Player/TATCharacter.h"

// ose
#include "Utl/OSEShapeCollisionTrackerComponent.h"

// ue4
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAreaVolume)

ATATAreaVolume::ATATAreaVolume()
{
   PrimaryActorTick.bCanEverTick = false;
   PrimaryActorTick.bStartWithTickEnabled = false;

   _shapeCollisionTrackerComponent = CreateDefaultSubobject<UOSEShapeCollisionTrackerComponent>(TEXT("_shapeCollisionTrackerComponent"));
   _shapeCollisionTrackerComponent->OnlyRunOnAuthority = false;
}

#if WITH_EDITOR
void ATATAreaVolume::CheckForErrors()
{
   Super::CheckForErrors();

   // Avoid checking class defaults
   if (!HasAnyFlags(RF_ClassDefaultObject))
   {
      FMessageLog msgLog(FName("MapCheck"));

      if (!GetAreaInfo().IsDataValid())
      {
         msgLog.Error()
            ->AddToken(FUObjectToken::Create(this))
            ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("TATAreaVolume \"%s\": Incomplete AreaInfo definition! Missing either Name or Area Tag"), *GetName()))));
      }
   }
}
#endif // WITH_EDITOR

void ATATAreaVolume::NotifyActorBeginOverlap(AActor* otherActor)
{
   Super::NotifyActorBeginOverlap(otherActor);

   if (HasAuthority())
   {
      if (ATATCharacter* character = Cast<ATATCharacter>(otherActor))
      {
         character->ServerSetAreaInfo(Info);
      }
   }
}

void ATATAreaVolume::NotifyActorEndOverlap(AActor* otherActor)
{
   Super::NotifyActorEndOverlap(otherActor);
   
   if (HasAuthority())
   {
      if (ATATCharacter* character = Cast<ATATCharacter>(otherActor))
      {
         character->ServerClearAreaInfo();
      }
   }
}

