// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/OSEFixupMapActorsCommandlet.h"

// ue
#include "EngineUtils.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "LevelInstance/LevelInstanceInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEFixupMapActorsCommandlet)
DEFINE_LOG_CATEGORY_STATIC(LogOSEFixupMapActorsCommandlet, Log, All);

void UOSEFixupMapActorsCommandlet ::PerformOperation(bool includeActorsInLevelInstances, TArray<UPackage*>& outPackagesToSave, TArray<FString>& levelInstancePackagePathsWarrantingOperation)
{
   const ULevelInstanceSubsystem* levelInstanceSubsystem = GWorld->GetSubsystem<ULevelInstanceSubsystem>();

   // Loop over each actor of specified subclasses, marking levels (i.e. owning package) to resave
   const TArray<FOSEMapOperationCommandletActorResult> actorrResults = _FindActorsOfSubclasses(_classesToFixup, includeActorsInLevelInstances);
   for (const FOSEMapOperationCommandletActorResult& actorResult : actorrResults)
   {
      for (AActor* actor : actorResult.Actors)
      {
         // If actor occupies a level instance, cache its occupying level instance so we can return its package for a subsequent pass
         if (actorResult.LevelInstanceId.IsValid())
         {
            continue;
         }

         if (FixupMapActor(actor))
         {
            // Only save non-level instance packages in this pass
            outPackagesToSave.AddUnique(actor->GetPackage());
         }
      }

      if (actorResult.LevelInstanceId.IsValid() && includeActorsInLevelInstances && levelInstanceSubsystem)
      {
         // Return package paths for level instance + any child instance for a subsequent pass
         const ILevelInstanceInterface* levelInstance = levelInstanceSubsystem->GetLevelInstance(actorResult.LevelInstanceId);
         levelInstancePackagePathsWarrantingOperation.AddUnique(levelInstance->GetWorldAssetPackage());
         levelInstanceSubsystem->ForEachLevelInstanceChild(levelInstance, true, [&](const ILevelInstanceInterface* levelInstanceChild)
            {
               levelInstancePackagePathsWarrantingOperation.AddUnique(levelInstanceChild->GetWorldAssetPackage());
               return true;
            });
      }
   }
}

TArray<FOSEMapOperationCommandletActorResult> UOSEFixupMapActorsCommandlet ::_FindActorsOfSubclasses(const TArray<TSubclassOf<AActor>>& subclasses, bool includeActorsInLevelInstance) const
{
   TArray<FOSEMapOperationCommandletActorResult> results;
   const ULevelInstanceSubsystem* levelInstanceSubsystem = GWorld->GetSubsystem<ULevelInstanceSubsystem>();

   for (const TSubclassOf<AActor>& actorClassToResave : subclasses)
   {
      for (TActorIterator<AActor> actorIt(GWorld, actorClassToResave); actorIt; ++actorIt)
      {
         AActor* actor = *actorIt;

         // Skip actors in level instances if specified
         const ILevelInstanceInterface* levelInstance = nullptr;
         if (levelInstanceSubsystem)
         {
            levelInstance = levelInstanceSubsystem->GetParentLevelInstance(actor);
            if (levelInstance && !includeActorsInLevelInstance)
            {
               continue;
            }
         }

         // Find or create entry for level instance (including a bucket for actors not occupying a level instance)
         const FLevelInstanceID levelInstanceId = levelInstance ? levelInstance->GetLevelInstanceID() : FLevelInstanceID();
         FOSEMapOperationCommandletActorResult* actorResult = results.FindByKey(levelInstanceId);
         if (!actorResult)
         {
            const int32 index = results.Emplace(levelInstanceId);
            actorResult = &results[index];
         }

         // Add actor to bucket
         actorResult->Actors.Add(actor);

         UE_LOG(LogOSEFixupMapActorsCommandlet, Verbose, TEXT("Detected actor of type %s: %s (level = %s)")
            , *actorClassToResave->GetName()
            , *actor->GetName()
            , levelInstance ? *levelInstance->GetWorldAssetPackage() : *actor->GetLevel()->GetPackage()->GetName());
      }
   }
   
   return results;
}
