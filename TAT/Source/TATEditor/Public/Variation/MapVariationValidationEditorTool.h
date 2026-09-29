// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "GameFramework/TATDifficulty.h"

// ose
#include "Editor/OSEBaseEditorTool.h"

// ue4

#include "MapVariationValidationEditorTool.generated.h"

class UTATSpawnDataAsset;
class FMessageLog;
class FTATSpawnToolOutputMode;
class UTATSceneVariantConfig;
class UTATQuestGraph;
struct FGameplayTag;
struct FTATSceneTraitWithLimit;

UENUM()
enum class ETATSimDifficulty : uint8
{
   Random,
   Specific
};

UCLASS(Config = EditorPerProjectUserSettings)
class TATEDITOR_API UTATMapVariationValidationEditorTool : public UOSEBaseEditorTool
{
   GENERATED_BODY()

public:
   UTATMapVariationValidationEditorTool();

   // from UOSEBaseEditorTool
   virtual void InitEditorTool() override;

   UFUNCTION(Exec, Category = "Validate")
   void ValidateSpawnData();

   UPROPERTY(Config, EditAnywhere, Category = "Simulate")
   int32 SimulationCount = 1000;

   UPROPERTY(Config, EditAnywhere, Category = "Simulate")
   int32 OverrideSeed = INDEX_NONE;

   UPROPERTY(Config, EditAnywhere, Category = "Simulate")
   bool ShowVariantPhases = false;

   UPROPERTY(Config, EditAnywhere, Category = "Simulate")
   bool ShowVariantFrequencies = false;

   UPROPERTY(Config, EditAnywhere, Category = "Simulate")
   bool ShowQuestSpawnerFrequencies = false;

   UPROPERTY(Config, EditAnywhere, Category = "Simulate")
   ETATSimDifficulty DifficultyType = ETATSimDifficulty::Random;

   UPROPERTY(Config, EditAnywhere, Category = "Simulate", meta = (EditCondition = "DifficultyType == ETATSimDifficulty::Specific", EditConditionHides))
   ETATDifficulty Difficulty = ETATDifficulty::Normal;

   UPROPERTY(EditAnywhere, Category = "Simulate")
   TArray<TObjectPtr<UTATSceneVariantConfig>> VariantOverrides;

   UPROPERTY(EditAnywhere, Category = "Simulate")
   TObjectPtr<UTATQuestGraph> QuestGraphOverride;

   UPROPERTY(EditAnywhere, Category = "Simulate", meta= (Categories="SceneTraitCategory"))
   TArray<FGameplayTag> ExtraTraits;

   UPROPERTY(EditAnywhere, Category = "Simulate")
   TArray<FTATSceneTraitWithLimit> LimitedTraits;

   // Locations for quest spawning
   // Not using quests directly, as that tries to load loot info when not available, and didn't seem worth it
   UPROPERTY(EditAnywhere, Category = "Simulate", meta = (Categories = "QuestLocation"))
   TArray<FGameplayTag> QuestSpawnLocations;

   UPROPERTY(EditAnywhere, Category = "Simulate")
   bool ShowSelectedMissionTagStats = true;

   UFUNCTION(Exec, Category = "Simulate")
   void SimulateSpawning();

   UFUNCTION(Exec, Category = "Simulate (Advanced)")
   void GenerateRawSequences();
   UFUNCTION(Exec, Category = "Simulate (Advanced)")
   void SimulateSpawningLog();


   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
};
