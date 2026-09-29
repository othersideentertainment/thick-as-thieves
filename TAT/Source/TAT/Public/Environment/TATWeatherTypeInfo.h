// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Common/TATGameplayTagTableRow.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataTable.h"
#include "TATWeatherTypeInfo.generated.h"

class ATATWeatherPreset;

/// The gameplay-specific data for each weather type
USTRUCT(BlueprintType)
struct TAT_API FTATWeatherTypeMetadata
{
   GENERATED_BODY()
   
   /// How far NPCs can see inside
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weather Type Metadata", Meta = (Categories = "Weather.Visibility"))
   FGameplayTag VisibilityIndoors;
   /// How far NPCs can see outside
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weather Type Metadata", Meta = (Categories = "Weather.Visibility"))
   FGameplayTag VisibilityOutdoors;

   /// How far sound carries
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weather Type Metadata", Meta = (Categories = "Weather.SoundCarry"))
   FGameplayTag SoundCarryIndoors;
   /// How far sound carries
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Weather Type Metadata", Meta = (Categories = "Weather.SoundCarry"))
   FGameplayTag SoundCarryOutdoors;
};

UENUM(BlueprintType)
enum class ETATWeatherLevelFilter : uint8
{
   AllowAll            UMETA(DisplayName = "Allow in All Levels", Tooltip = "The weather type can be used in ALL levels"),
   AllowNone           UMETA(DisplayName = "Disabled", Tooltip = "The weather type can NOT be used ANYWHERE"),
   UseAllowList        UMETA(Tooltip = "The weather type can ONLY be used with the levels in the filter list"),
   UseDenyList         UMETA(Tooltip = "The weather type can be used with ANY level EXCEPT the levels in the filter list"),
};

/// The designer-oriented data related to weather types
USTRUCT(BlueprintType, meta=(RowNameTag=WeatherType))
struct TAT_API FTATWeatherTypeInfo : public FTATGameplayTagTableRow
{
   GENERATED_BODY()

public:
   /// The gameplay tag representing this weather type
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Type", Meta = (Categories = "Weather.Type"))
   FGameplayTag WeatherType;

   /// The descriptive name of this weather type that will appear in the UI
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Type")
   FText Label;

   /// The "canonical" preset class for this weather type. This can be customized per-level by adding an override in that level's weather manager
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Type")
   TSoftClassPtr<ATATWeatherPreset> DefaultPreset;

   /// Gameplay-relevant mutators
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Weather Type", Meta = (ShowOnlyInnerProperties))
   FTATWeatherTypeMetadata GameplayMetadata;

   /// If enabled, this weather type can be used in any level
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Level Filter")
   ETATWeatherLevelFilter LevelFilterMode = ETATWeatherLevelFilter::AllowAll;

   /// The level filter list. This is either a list of levels to allow or deny depending on the level filter mode.
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Level Filter", Meta = (EditCondition = "LevelFilterMode == ETATWeatherLevelFilter::UseAllowList || LevelFilterMode == ETATWeatherLevelFilter::UseDenyList"))
   TArray<TSoftObjectPtr<UWorld>> LevelFilterList;

   bool IsValid() const { return WeatherType.IsValid(); }

   /// Checks a level to make sure it passes any restrictions on using it in a particular level
   bool IsWeatherTypeAllowedInLevel(UWorld* world) const;
   bool IsWeatherTypeAllowedInLevel(const TSoftObjectPtr<UWorld>& world) const;
};
