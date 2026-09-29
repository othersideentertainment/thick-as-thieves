// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/OSEMapCheckCommandlet.h"

// ose

// ue4
#include "FileHelpers.h"
#include "LevelInstance/LevelInstanceSubsystem.h"
#include "Logging/LogScopedVerbosityOverride.h"
#include "Misc/FeedbackContext.h"
#include "Settings/ProjectPackagingSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEMapCheckCommandlet)


UOSEMapCheckCommandlet::UOSEMapCheckCommandlet(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

int UOSEMapCheckCommandlet::_RunOSECommandlet(const FString& fullCommandLine)
{
   _RunMapChecks();
   return 0;
}

void UOSEMapCheckCommandlet::_RunMapChecks()
{
   UProjectPackagingSettings* packagingSettings = Cast<UProjectPackagingSettings>(UProjectPackagingSettings::StaticClass()->GetDefaultObject());
   check(packagingSettings);
   
   for(const FFilePath& map : packagingSettings->MapsToCook)
   {
      if (MapChecksToSkip.FindByPredicate([map](const FFilePath& skipMap)
      {
         return skipMap.FilePath == map.FilePath;
      }))
      {
         UE_LOG(LogOSECommandlet, Display, TEXT("Skipping map %s ..."), *map.FilePath);
         continue;
      }

      UE_LOG(LogOSECommandlet, Display, TEXT("Loading %s ..."), *map.FilePath);
      
      const bool kLoadAsTemplate = false;
      if (!FEditorFileUtils::LoadMap(*map.FilePath, kLoadAsTemplate))
      {
         UE_LOG(LogOSECommandlet, Error, TEXT("Failed to load %s!"), *map.FilePath);
         continue;
      }

      // Normally, the editor will start streaming levels on the first tick. Update the streaming state manually instead
      if (ULevelInstanceSubsystem* LevelInstanceSubsystem = GWorld->GetSubsystem<ULevelInstanceSubsystem>())
      {
         LevelInstanceSubsystem->OnUpdateStreamingState();
      }

      // Make sure all requested sublevels are finished loading before we run map check
      GWorld->FlushLevelStreaming();

      UE_LOG(LogOSECommandlet, Display, TEXT("Map checking %s ..."), *map.FilePath);
      GEditor->Exec(GWorld, TEXT("MAP CHECK"));
   }
}
