// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "WorldMap/TATWorldMapFunctionLibrary.h"

// tat
#include "GameFramework/TATWorldSettings.h"
#include "WorldMap/TATTransientMapActor.h"
#include "WorldMap/TATWorldMapBoundary.h"

// ue
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWorldMapFunctionLibrary)
DEFINE_LOG_CATEGORY_STATIC(LogTATWorldMapFunctionLibrary, Log, All);

namespace TATWorldMapFunctionLibraryUtils
{
   FVector GenerateRandomOffset(float randomOffsetDistance)
   {
      constexpr float circleRadius = 1.f;
      const FVector2D randomOffset = FMath::RandPointInCircle(circleRadius) * randomOffsetDistance;
      return FVector(randomOffset.X, randomOffset.Y, 0.f);
   }

   FVector GetOffsetWorldLocation(const UObject* const worldContext, const FVector startLocation, const float randomOffsetDistance)
   {
      const auto& worldSettings = ATATWorldSettings::Get(worldContext);
      const ATATWorldMapBoundary* worldMapBoundary = worldSettings.WorldMapBoundary;
      if (!worldMapBoundary)
      {
         return startLocation;
      }

      const FBox boundaryBox = worldMapBoundary->GetMapAreaBoundingBox();

      FVector offsetLocation = startLocation  + TATWorldMapFunctionLibraryUtils::GenerateRandomOffset(randomOffsetDistance);
      offsetLocation.X = FMath::Clamp(offsetLocation.X, boundaryBox.Min.X, boundaryBox.Max.X);
      offsetLocation.Y = FMath::Clamp(offsetLocation.Y, boundaryBox.Min.Y, boundaryBox.Max.Y);
      return offsetLocation;
   }

   ATATTransientMapActor* AuthorityInternalSpawnTransientMapActor(
      const UObject* worldContext,
      FVector worldLocation,
      float actorLifespan,
      FGameplayTag transientMapActorIdentifier,
      FGameplayTag mapSpriteOverride,
      AActor* owner = nullptr)
   {
      if (!worldContext)
      {
         UE_LOG(LogTATWorldMapFunctionLibrary, Error, TEXT("Called with invalid worldContext!"));
         return nullptr;
      }
      if (!transientMapActorIdentifier.IsValid())
      {
         UE_LOG(LogTATWorldMapFunctionLibrary, Error, TEXT("Called with invalid transientMapActorIdentifier!"));
         return nullptr;
      }
      UWorld* world = worldContext->GetWorld();
      check(world);

      const FTransform spawnTransform(FRotator::ZeroRotator, worldLocation, FVector::OneVector);
      ATATTransientMapActor* transientMapActor = world->SpawnActorDeferred<ATATTransientMapActor>(ATATTransientMapActor::StaticClass(), spawnTransform, owner);
      if (!transientMapActor)
      {
         UE_LOG(LogTATWorldMapFunctionLibrary, Error, TEXT("Failed to spawn actor at location %s!"), *worldLocation.ToString());
         return nullptr;
      }

      // If provided an owner, limit net-relevancy to them (ensures it only replicates to the provided player if specified)
      if (owner != nullptr)
      {
         transientMapActor->bOnlyRelevantToOwner = true;
      }
      transientMapActor->TransientMapActorIdentifier = transientMapActorIdentifier;
      transientMapActor->MapSpriteOverride = mapSpriteOverride;

      transientMapActor->SetLifeSpan(actorLifespan);
      constexpr bool isDefaultTransform = true;
      transientMapActor->FinishSpawning(spawnTransform, isDefaultTransform);

      return transientMapActor;
   }
}

ATATTransientMapActor* UTATWorldMapFunctionLibrary::AuthoritySpawnTransientMapActorForAllPlayers(
   const UObject* worldContext, 
   FGameplayTag transientMapActorIdentifier, 
   FGameplayTag mapSpriteOverride, 
   FVector worldLocation, 
   float randomOffsetDistance, 
   float actorLifespan)
{
   const FVector spawnLocation = TATWorldMapFunctionLibraryUtils::GetOffsetWorldLocation(worldContext, worldLocation, randomOffsetDistance);
   return TATWorldMapFunctionLibraryUtils::AuthorityInternalSpawnTransientMapActor(worldContext, spawnLocation, actorLifespan, transientMapActorIdentifier, mapSpriteOverride);
}

ATATTransientMapActor* UTATWorldMapFunctionLibrary::AuthoritySpawnTransientMapActorForPlayer(
   const UObject* worldContext, 
   APlayerController* playerController, 
   FGameplayTag transientMapActorIdentifier, 
   FGameplayTag mapSpriteOverride, 
   FVector worldLocation, 
   float randomOffsetDistance, 
   float actorLifespan)
{
   if (!playerController)
   {
      UE_LOG(LogTATWorldMapFunctionLibrary, Error, TEXT("AuthoritySpawnTransientMapActorForPlayer() called with invalid PlayerController!"));
      return nullptr;
   }

   AActor* owner = playerController;
   const FVector spawnLocation = TATWorldMapFunctionLibraryUtils::GetOffsetWorldLocation(worldContext, worldLocation, randomOffsetDistance);
   return TATWorldMapFunctionLibraryUtils::AuthorityInternalSpawnTransientMapActor(worldContext, spawnLocation, actorLifespan, transientMapActorIdentifier, mapSpriteOverride, owner);
}
