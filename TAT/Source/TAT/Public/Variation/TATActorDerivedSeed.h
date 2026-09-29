// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATActorDerivedSeed.generated.h"

class AActor;

namespace TATActorDerivedSeed
{
   // Deterministically generates a seed from the current map seed, the actor, and the provided map seed
   // for use in seeding a random stream without having to explicitly replicate it (and potentially flush
   // dormancy).
   //
   // The initial use-case is for the selection of a lock variant for locked actors (which could be done
   // just in time).
   //
   // Usage:
   //
   // The actor must be one or both of:
   // 1. Placed in a level (and thus have a stable name on client and server)
   // 2. Stay in the same place (so the location can be used)
   // 
   // If there are dynamically spawned mobile actors, they might as well just replicate something from,
   // the server anyways, but something could be added later if there is a use-case for using the same
   // codepath.
   //
   // This function should be safe to use after BeginPlay, and it will hit an ensure if used too early.
   int32 GetDerivedSeedForActor(const AActor* actor, const TCHAR* context);

   int32 GetMapSeed(const UObject* worldContext);
}

// TODO: move to a different BPFL if there is an appropriate one?
UCLASS()
class TAT_API UTATDerivedSeedFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()
   
public:
   // Deterministically generates a seed from the current map seed, the actor, and the provided map seed
   // for use in seeding a random stream without having to explicitly replicate it (and potentially flush
   // dormancy).
   //
   // The initial use-case is for the selection of a lock variant for locked actors (which could be done
   // just in time).
   //
   // Usage:
   //
   // The actor must be one or both of:
   // 1. Placed in a level (and thus have a stable name on client and server)
   // 2. Stay in the same place (so the location can be used)
   // 
   // If there are dynamically spawned mobile actors, they might as well just replicate something from,
   // the server anyways, but something could be added later if there is a use-case for using the same
   // codepath.
   //
   // This function should be safe to use after BeginPlay, and it will hit an ensure if used too early.
   UFUNCTION(BlueprintCallable, Category="TAT|MapVariation", meta=(DefaultToSelf="actor"))
   static FRandomStream GetDerivedRandomStreamForActor(const AActor* actor, FName context);
};
