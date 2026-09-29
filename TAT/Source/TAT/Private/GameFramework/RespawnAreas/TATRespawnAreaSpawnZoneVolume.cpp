// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/RespawnAreas/TATRespawnAreaSpawnZoneVolume.h"

// tat
#include "Character/TATCharacterAIBase.h"

// ose
#include "Utl/OSEShapeCollisionTrackerComponent.h"

// ue
#include "Components/BoxComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATRespawnAreaSpawnZoneVolume)

ATATRespawnAreaSpawnZoneVolume::ATATRespawnAreaSpawnZoneVolume()
{
   PrimaryActorTick.bCanEverTick = false;

   _boxShapeCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxShapeCollision"));
   _boxShapeCollision->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
   _boxShapeCollision->SetMobility(EComponentMobility::Static);

   RootComponent = _boxShapeCollision;

   _collisionTrackerComponent = CreateDefaultSubobject<UOSEShapeCollisionTrackerComponent>(TEXT("CollisionShapeTracker"));
   _collisionTrackerComponent->RequiredTimeInsideOfShape = 0.0f;
}

void ATATRespawnAreaSpawnZoneVolume::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      _collisionTrackerComponent->OnActorEnteredShape.AddUniqueDynamic(this, &ThisClass::_AuthorityOnActorEnterShape);
      _collisionTrackerComponent->OnActorExitedShape.AddUniqueDynamic(this, &ThisClass::_AuthorityOnActorExitShape);
   }
}

FVector ATATRespawnAreaSpawnZoneVolume::GetRespawnMarkerLocation() const
{
   return GetActorTransform().TransformPosition(_respawnMarkerOffset);
}

void ATATRespawnAreaSpawnZoneVolume::_AuthorityOnActorEnterShape(AActor* actor)
{
   if (actor && actor->IsA<ATATCharacterAIBase>())
   {
      _trackedAICharactersWithinShape++;
      OnSpawnZoneAITrackedCountChanged.Broadcast();
   }
}

void ATATRespawnAreaSpawnZoneVolume::_AuthorityOnActorExitShape(AActor* actor)
{
   if (actor && actor->IsA<ATATCharacterAIBase>())
   {
      _trackedAICharactersWithinShape--;
      OnSpawnZoneAITrackedCountChanged.Broadcast();
   }
}
