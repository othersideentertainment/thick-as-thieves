// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"

#include "TATWorldMapFunctionLibrary.generated.h"

class APlayerController;
class ATATTransientMapActor;

UCLASS()
class TAT_API UTATWorldMapFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   /// Creates a map icon at the provided location (optionally offset in random direction by provided amount) for all players
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (WorldContext="worldContext"))
   static ATATTransientMapActor* AuthoritySpawnTransientMapActorForAllPlayers(
      const UObject* worldContext, 
      UPARAM(meta = (Categories = "TransientMapActor")) FGameplayTag transientMapActorIdentifier, 
      UPARAM(meta = (Categories = "MapSprite")) FGameplayTag mapSpriteOverride, 
      FVector worldLocation, 
      float randomOffsetDistance = 0.f, 
      float actorLifespan = 0.f);

   /// Creates a map icon at the provided location (optionally offset in random direction by provided amount) for given player.
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, meta = (WorldContext="worldContext"))
   static ATATTransientMapActor* AuthoritySpawnTransientMapActorForPlayer(
      const UObject* worldContext, 
      APlayerController* playerController, 
      UPARAM(meta = (Categories = "TransientMapActor")) FGameplayTag transientMapActorIdentifier, 
      UPARAM(meta = (Categories = "MapSprite")) FGameplayTag mapSpriteOverride, 
      FVector worldLocation, 
      float randomOffsetDistance = 0.f, 
      float actorLifespan = 0.f);
};
