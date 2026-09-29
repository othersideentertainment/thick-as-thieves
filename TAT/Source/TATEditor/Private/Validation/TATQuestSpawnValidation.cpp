// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Validation/TATQuestSpawnValidation.h"

// tat editor
#include "TATEditorModuleSettings.h"

// tat
#include "GameFramework/TATWorldSettings.h"
#include "Variation/TATSpawnData.h"
#include "Quests/Spawn/TATQuestActorSpawner.h"
#include "Quests/Spawn/TATQuestSpawnUtils.h"

// ue
#include "Logging/MessageLog.h"

namespace TATQuestSpawnValidation
{
   static bool ShouldValidate(UWorld* world)
   {
      const ATATWorldSettings& worldSettings = ATATWorldSettings::Get(world);
      if (UTATSpawnDataAsset* spawnData = worldSettings.SpawnData)
      {
         return spawnData->ValidateQuestSpawners;
      }

      return false;
   }
}

void TATQuestSpawnValidation::TryValidate(UWorld* world, FMessageLog& msgLog)
{
   if (!ShouldValidate(world))
   {
      return;
   }

   TArray<TObjectPtr<UTATQuestActorSpawnerComponent>> spawners;
   TATQuestSpawnUtils::ScrapeSpawnersInLevel(world, spawners);

   TMap<FGameplayTag, int32> fallbackSpawnersByLocation;
   TMap<FGameplayTag, int32> variantSpawnersByLocation;

   for (UTATQuestActorSpawnerComponent* spawner : spawners)
   {
      if (IsValid(spawner) && spawner->IsEnabled())
      {
         TMap<FGameplayTag, int32>& mapToAddTo = spawner->GetSceneRequirement().IsNone() ? fallbackSpawnersByLocation : variantSpawnersByLocation;
         mapToAddTo.FindOrAdd(spawner->GetQuestLocationTag()) += 1;
      }
   }

   const UTATEditorModuleSettings& editorSettings = UTATEditorModuleSettings::Get();
   const int32 defaultMinSpawners = editorSettings.DefaultMinFallbackQuestSpawners;

   for (const FTATQuestSpawnLocationValidationSetting& questLocation : editorSettings.RequiredQuestSpawnLocations)
   {
      const int32 numFallbackSpawners = fallbackSpawnersByLocation.FindRef(questLocation.QuestLocationTag);
      const int32 minFallbackSpawners = questLocation.UseMinFallbackSpawnersOverride ? questLocation.MinFallbackSpawnersOverride : defaultMinSpawners;
      if (numFallbackSpawners < minFallbackSpawners)
      {
         msgLog.Error(FText::FromString(FString::Printf(TEXT("Not enough fallback quest spawners without scene requirements in map for quest location %s (Needed at least %d, but had %d)\nThese are needed to handle the worst case when variants are not selected. (There are %d variant-specific spawners for that location, but those may not always be selectable)"),
            *questLocation.QuestLocationTag.ToString(),
            minFallbackSpawners,
            numFallbackSpawners,
            variantSpawnersByLocation.FindRef(questLocation.QuestLocationTag))));
      }
   }
}
