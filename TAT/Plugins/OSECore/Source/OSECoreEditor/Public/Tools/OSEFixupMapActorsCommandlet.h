// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Tools/OSEMapOperationBaseCommandlet.h"

// ue
#include "LevelInstance/LevelInstanceTypes.h"

#include "OSEFixupMapActorsCommandlet.generated.h"

/// Class used for bucketing relevant actors by their occupying level instance (if any)
struct OSECOREEDITOR_API FOSEMapOperationCommandletActorResult
{
   FOSEMapOperationCommandletActorResult(FLevelInstanceID levelInstanceId) : LevelInstanceId(levelInstanceId) {}

   TArray<AActor*> Actors;

   FLevelInstanceID LevelInstanceId;

   FORCEINLINE bool operator==(FLevelInstanceID levelInstanceId) const { return levelInstanceId == LevelInstanceId; }
};

/// A commandlet for automating some fixup for all actors (derived from specified classes) across a collection of maps
UCLASS()
class OSECOREEDITOR_API UOSEFixupMapActorsCommandlet : public UOSEMapOperationBaseCommandlet
{
   GENERATED_BODY()

protected:
   // From UOSEMapOperationBaseCommandlet
   virtual void PerformOperation(bool includeActorsInLevelInstances, TArray<UPackage*>& outPackagesToSave, TArray<FString>& levelInstancePackagePathsWarrantingOperation) override final;

   /// Derived commandlets should implement this to perform whatever fixup is desired on each actor instance of matching class across maps
   virtual bool FixupMapActor(AActor* actor) { unimplemented(); return false; };

private:
   /// Iterates over actors in currently-loaded map, returning a collection of actors derived from provided subclasses bucketed by their occupying level instance (if any)
   TArray<FOSEMapOperationCommandletActorResult> _FindActorsOfSubclasses(const TArray<TSubclassOf<AActor>>& subclasses, bool includeActorsInLevelInstance) const;

private:
   /// Used to filter actors that _FixupMapActor() should run on
   UPROPERTY(Config, meta = (AllowAbstract = true))
   TArray<TSubclassOf<AActor>> _classesToFixup;
};
