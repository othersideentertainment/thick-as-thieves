// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/OSEMapOperationBaseCommandlet.h"

// ose
#include "Tools/OSEAssetRegistryUtl.h"

// ue
#include "FileHelpers.h"
#include "LevelInstance/LevelInstanceInterface.h"
#include "LevelInstance/LevelInstanceSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEMapOperationBaseCommandlet)
DEFINE_LOG_CATEGORY_STATIC(LogOSEMapOperationCommandletBase, Log, All);

int UOSEMapOperationBaseCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{
   TArray<FString> tokens;
   TArray<FString> switches;
   TMap<FString, FString> params;
   ParseCommandLine(*fullCommandLine, tokens, switches, params);

   const bool operateOnLevelInstances = switches.Contains(TEXT("recursive"));
   TArray<UPackage*> packagesToSave;
   TArray<FString> levelInstancePathsToPerformOperationOn;

   // First loop through top-level maps, caching linked-via-level-instance maps for a second pass
   for (const FFilePath& filePath : _mapsToRunOn)
   {
      UE_LOG(LogOSEMapOperationCommandletBase, Display, TEXT("=== Performing operation on %s ==="), *filePath.FilePath);
      _PerformOperationAndSaveMap(filePath.FilePath, levelInstancePathsToPerformOperationOn, operateOnLevelInstances);
   }

   // Then (if specified) loop over referenced level instances, including their own linked instances
   if (operateOnLevelInstances)
   {
      TArray<FString> processedLevelInstances;
      while (!levelInstancePathsToPerformOperationOn.IsEmpty())
      {
         // Make sure to only process each instanced level once (in case it's reused across multiple levels)
         const FString levelInstancePackagePath = levelInstancePathsToPerformOperationOn.Pop();
         if (!processedLevelInstances.Contains(levelInstancePackagePath))
         {
            UE_LOG(LogOSEMapOperationCommandletBase, Display, TEXT("=== Performing operation on level instance %s ==="), *levelInstancePackagePath);
            _PerformOperationAndSaveMap(levelInstancePackagePath, levelInstancePathsToPerformOperationOn, operateOnLevelInstances);
            processedLevelInstances.Add(levelInstancePackagePath);
         }
      }
   }

   return 0;
}

bool UOSEMapOperationBaseCommandlet::_LoadMap(const FString& mapPath)
{
   UE_LOG(LogOSEMapOperationCommandletBase, Verbose, TEXT("Loading [%s] ..."), *mapPath);
      
   const bool kLoadAsTemplate = false;
   if (!FEditorFileUtils::LoadMap(*mapPath, kLoadAsTemplate))
   {
      UE_LOG(LogOSEMapOperationCommandletBase, Error, TEXT("Failed to load map path: [%s]!"), *mapPath);
      return false;
   }

   // Normally, the editor will start streaming levels on the first tick. Update the streaming state manually instead
   if (ULevelInstanceSubsystem* levelInstanceSubsystem = GWorld->GetSubsystem<ULevelInstanceSubsystem>())
   {
      levelInstanceSubsystem->OnUpdateStreamingState();
   }
   
   // Make sure all requested sublevels are finished loading before we run map check
   GWorld->FlushLevelStreaming();
   return true;
}

void UOSEMapOperationBaseCommandlet::_PerformOperationAndSaveMap(const FString& mapPath, TArray<FString>& levelInstancePathsToPerformOperationOn, bool operateOnLevelInstances)
{
   TArray<UPackage*> packagesToSave;

   if (_LoadMap(mapPath))
   {
      PerformOperation(operateOnLevelInstances, packagesToSave, levelInstancePathsToPerformOperationOn);
      if (!packagesToSave.IsEmpty())
      {
         OSEAssetRegistryUtl::SavePackages(packagesToSave);
         packagesToSave.Reset();
      }
   }
}
