// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue5
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "TATEditorModuleSettings.generated.h"

class UTATCharactersMetadata;

USTRUCT()
struct FTATPlayerStartValidationSettings
{
   GENERATED_BODY()

   UPROPERTY(Config, EditDefaultsOnly, Category = "PlayerStart Validation")
   float CapsuleHalfHeight = 100;

   UPROPERTY(Config, EditDefaultsOnly, Category = "PlayerStart Validation")
   float CapsuleRadius = 50;

   UPROPERTY(Config, EditDefaultsOnly, Category = "PlayerStart Validation")
   FCollisionProfileName CapsuleProfile;
};

USTRUCT()
struct FTATQuestSpawnLocationValidationSetting
{
   GENERATED_BODY()

   UPROPERTY(Config, EditDefaultsOnly, meta = (Categories = "QuestLocation"))
   FGameplayTag QuestLocationTag;

   // override the number of require spawners for the location, such as if it is only used for a single quest
   UPROPERTY(Config, EditDefaultsOnly, meta = (EditCondition="UseMinFallbackSpawnersOverride"))
   int32 MinFallbackSpawnersOverride = 0;

   UPROPERTY(Config, EditDefaultsOnly, meta = (InlineEditConditionToggle))
   bool UseMinFallbackSpawnersOverride = false;
};

UCLASS(Config = EditorSettings, DefaultConfig, Meta = (DisplayName = "[TAT] Editor Settings"))
class TATEDITOR_API UTATEditorModuleSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // static get
   static const UTATEditorModuleSettings& Get() { return *GetDefault<UTATEditorModuleSettings>(); }

   UPROPERTY(Config, EditDefaultsOnly, Category = "PlayerStart Validation")
   FTATPlayerStartValidationSettings PlayerStartCollision;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Interactable Validation")
   TArray<TSoftObjectPtr<UStaticMesh>> InteractableOnlyStaticMeshes;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Circular Asset Validation")
   TArray<FString> FilePathPrefixesToSkipCircularAssetValidation;

   // The default minimum number of fallback quest spawners for a quest location
   // This should generally be the max number of supported players in a match,
   // assuming that each player could be on a different quest that needs the same
   // location.
   // TODO: Should this be more directly tied to that max-player setting?
   UPROPERTY(Config, EditDefaultsOnly, Category = "Quest Spawner Validation")
   int32 DefaultMinFallbackQuestSpawners = 4;

   // The quest spawn locations that each match map should have
   UPROPERTY(Config, EditDefaultsOnly, Category = "Quest Spawner Validation")
   TArray<FTATQuestSpawnLocationValidationSetting> RequiredQuestSpawnLocations;
};
