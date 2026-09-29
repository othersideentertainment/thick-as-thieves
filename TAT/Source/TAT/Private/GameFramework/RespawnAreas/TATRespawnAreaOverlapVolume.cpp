// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "GameFramework/RespawnAreas/TATRespawnAreaOverlapVolume.h"

// tat
#include "GameFramework/RespawnAreas/TATRespawnAreaSpawnLocation.h"
#include "GameFramework/RespawnAreas/TATRespawnAreaSpawnZoneVolume.h"
#include "Player/TATPlayerController.h"

// ose
#include "Player/OSEPlayerCharacter.h"
#include "Utl/OSEShapeCollisionTrackerComponent.h"

// ue
#include "Components/BoxComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATRespawnAreaOverlapVolume)

ATATRespawnAreaOverlapVolume::ATATRespawnAreaOverlapVolume()
{
   PrimaryActorTick.bCanEverTick = false;

   _boxShapeCollision = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxShapeCollision"));
   _boxShapeCollision->SetCollisionProfileName(TEXT("OverlapOnlyPawn"));
   _boxShapeCollision->SetMobility(EComponentMobility::Static);

   RootComponent = _boxShapeCollision;

   _collisionTrackerComponent = CreateDefaultSubobject<UOSEShapeCollisionTrackerComponent>(TEXT("CollisionShapeTracker"));
   _collisionTrackerComponent->RequiredTimeInsideOfShape = 0.0f;
}

void ATATRespawnAreaOverlapVolume::BeginPlay()
{
   Super::BeginPlay();
   if (HasAuthority())
   {
      _collisionTrackerComponent->OnActorEnteredShape.AddUniqueDynamic(this, &ThisClass::_AuthorityOnActorEnterShape);
      _collisionTrackerComponent->OnActorExitedShape.AddUniqueDynamic(this, &ThisClass::_AuthorityOnActorExitShape);

      if (_spawnZoneVolume)
      {
         _spawnZoneVolume->OnSpawnZoneAITrackedCountChanged.AddUniqueDynamic(this, &ThisClass::_AuthorityOnTrackedAICountChanged);
      }
   }
}

void ATATRespawnAreaOverlapVolume::_AuthorityOnActorEnterShape(AActor* actor)
{
   if (const AOSEPlayerCharacter* playerCharacter = Cast<AOSEPlayerCharacter>(actor))
   {
      if (auto pc = playerCharacter->GetController<ATATPlayerController>())
      {
         pc->AuthorityAddRespawnArea(this);
      }
   }
}

void ATATRespawnAreaOverlapVolume::_AuthorityOnActorExitShape(AActor* actor)
{
   if (const AOSEPlayerCharacter* playerCharacter = Cast<AOSEPlayerCharacter>(actor))
   {
      if (auto pc = playerCharacter->GetController<ATATPlayerController>())
      {
         pc->AuthorityRemoveRespawnArea(this);
      }
   }
}

void ATATRespawnAreaOverlapVolume::_AuthorityOnTrackedAICountChanged()
{
   check(_spawnZoneVolume);
   const bool aiDetectedInSafeZone = _spawnZoneVolume->GetTrackedAICharactersWithinShape() > 0;
   if (aiDetectedInSafeZone != _aiDetectedWithinSafeZone)
   {
      _aiDetectedWithinSafeZone = aiDetectedInSafeZone;
      OnAIDetectedWithinSafeZoneChanged.Broadcast(this, _aiDetectedWithinSafeZone);
   }
}

ATATRespawnAreaSpawnLocation* ATATRespawnAreaOverlapVolume::GetValidPlayerStartForVolume(const APawn* pawnToSpawn) const
{
   UWorld* world = GetWorld();
   for (const TObjectPtr<ATATRespawnAreaSpawnLocation>& spawnPoint : _playerSpawnPoints)
   {
      if (spawnPoint == nullptr)
      {
         continue;
      }

      FVector actorLocation = spawnPoint->GetActorLocation();
      const FRotator actorRotation = spawnPoint->GetActorRotation();
      // This is effectively the same overlap checks that the game mode does
      if (world->EncroachingBlockingGeometry(pawnToSpawn, actorLocation, actorRotation) == false)
      {
         return spawnPoint;
      }
   }
   return nullptr;
}

FVector ATATRespawnAreaOverlapVolume::GetRespawnMarkerLocation() const
{
   if(ensure(_spawnZoneVolume))
   {
      return _spawnZoneVolume->GetRespawnMarkerLocation();
   }

   return GetActorLocation();
}
