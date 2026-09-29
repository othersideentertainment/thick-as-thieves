// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Developer/TATWeatherSettings.h"

// tat
#include "TATGameInstance.h"
#include "Developer/TATEditorSettings.h"
#include "Environment/TATWeatherManager.h"
#include "Environment/TATWeatherTypeInfo.h"
#include "Environment/TATWeatherPreset.h"
#include "GameFramework/TATWorldSettings.h"
#include "Settings/TATMatchSettingsBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWeatherSettings)

DEFINE_LOG_CATEGORY_STATIC(LogTATWeatherSettings, Log, All);

const FTATWeatherTypeInfo* UTATWeatherSettings::FindWeatherTypeInfo(FGameplayTag weatherType, const UDataTable* weatherDataTable) const
{
   if (weatherDataTable == nullptr)
   {
      weatherDataTable = WeatherTypeDataTable.LoadSynchronous();
      check(weatherDataTable != nullptr);
   }
   check(weatherDataTable != nullptr);
   check(weatherDataTable->GetRowStruct() == FTATWeatherTypeInfo::StaticStruct());
   constexpr const TCHAR* contextString = TEXT("UTATWeatherSettings::FindWeatherTypeInfo");
   constexpr bool warnIfMissing = false;
   return weatherDataTable->FindRow<FTATWeatherTypeInfo>(weatherType.GetTagName(), contextString, warnIfMissing);
}

// static
bool UTATWeatherSettings::BP_FindWeatherTypeInfo(FGameplayTag weatherType, FTATWeatherTypeInfo& weatherTypeInfo)
{
   if (const FTATWeatherTypeInfo* row = UTATWeatherSettings::Get().FindWeatherTypeInfo(weatherType))
   {
      weatherTypeInfo = *row;
      return true;
   }
   weatherTypeInfo = FTATWeatherTypeInfo{};
   return false;
}

// static
void UTATWeatherSettings::BP_GetAllWeatherTypes(TArray<FGameplayTag>& weatherTypes)
{
   weatherTypes.Reset();

   const UDataTable* weatherDataTable = UTATWeatherSettings::Get().WeatherTypeDataTable.LoadSynchronous();
   if (weatherDataTable == nullptr)
   {
      return;
   }

   check(weatherDataTable->GetRowStruct() == FTATWeatherTypeInfo::StaticStruct());
   constexpr const TCHAR* contextString = TEXT("UTATWeatherSettings::BP_GetAllWeatherTypes");
   weatherTypes.Reserve(weatherDataTable->GetRowMap().Num());
   weatherDataTable->ForeachRow<FTATWeatherTypeInfo>(contextString, [&weatherTypes](const FName& key, const FTATWeatherTypeInfo& info)
   {
      weatherTypes.Add(info.WeatherType);
   });
}

// static
TSoftClassPtr<ATATWeatherPreset> UTATWeatherSettings::GetWeatherPresetClassForWeatherType(const UObject* worldContext, FGameplayTag weatherType)
{
   const UTATWeatherSettings& weatherSettings = UTATWeatherSettings::Get();
   UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::LogAndReturnNull);
   if (world == nullptr)
   {
      return weatherSettings.FallbackWeatherPreset;
   }

   // If we have an override configured, just use that
   ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings());
   if (worldSettings != nullptr && worldSettings->WeatherManager != nullptr)
   {
      if (const TSoftClassPtr<ATATWeatherPreset>* presetOverride = worldSettings->WeatherManager->PresetOverrides.Find(weatherType))
      {
         if (!presetOverride->IsNull())
         {
            return *presetOverride;
         }
      }
   }

   // Look up the weather type metadata to find the preset class
   const FTATWeatherTypeInfo* weatherTypeInfo = weatherSettings.FindWeatherTypeInfo(weatherType);
   if (weatherTypeInfo != nullptr && !weatherTypeInfo->DefaultPreset.IsNull())
   {
      if (weatherTypeInfo->IsWeatherTypeAllowedInLevel(world))
      {
         return weatherTypeInfo->DefaultPreset;
      }
      else
      {
         UE_LOG(LogTATWeatherSettings, Error, TEXT("Weather preset '%s' is not configured to be loaded in level '%s' in the weather types data table"),
            *weatherType.ToString(), *world->GetName());
      }
   }

   return weatherSettings.FallbackWeatherPreset;
}

// static
TSoftClassPtr<ATATWeatherPreset> UTATWeatherSettings::GetCurrentWeatherPresetClass(const UObject* worldContext, bool logOrEnsureWhenReturningFallbacks)
{
   const UTATWeatherSettings& weatherSettings = UTATWeatherSettings::Get();
   UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull);

#if WITH_EDITOR
   const UTATEditorSettings& editorSettings = UTATEditorSettings::Get();
   // Weather type override has the highest precedence in the editor.
   // This requires looking up the correct preset for that type, possibly based on the current level.
   switch (editorSettings.WeatherOverride)
   {
   case ETATEditorSettingsWeatherOverrideMode::OverrideWeatherType:
      if (editorSettings.WeatherTypeOverride.IsValid())
      {
         return GetWeatherPresetClassForWeatherType(worldContext, editorSettings.WeatherTypeOverride);
      }
      break;
   case ETATEditorSettingsWeatherOverrideMode::OverrideWeatherPreset:
      if (!editorSettings.WeatherPresetOverride.IsNull())
      {
         return editorSettings.WeatherPresetOverride;
      }
      break;
   case ETATEditorSettingsWeatherOverrideMode::UseLevelDefaultEditorPreset:
      if (world != nullptr)
      {
         ATATWorldSettings* worldSettings = (world != nullptr) ? Cast<ATATWorldSettings>(world->GetWorldSettings()) : nullptr;
         if (worldSettings != nullptr && !worldSettings->EditorDefaultWeatherPreset.IsNull())
         {
            return worldSettings->EditorDefaultWeatherPreset;
         }
      }
      break;
   default:
      break;
   }
#endif

   // Try getting the weather type from the game instance's copy of match settings
   // Allow world or the game instance to be null (we could be calling this in the editor without being in PIE/simulate)
   if (world != nullptr && weatherSettings.MatchSettingsWeatherTypePropertyName != NAME_None)
   {
      if (UTATGameInstance* gameInstance = world->GetGameInstance<UTATGameInstance>())
      {
         FGameplayTag weatherType;
         if (gameInstance->GetMatchSettings().GetMatchSettingsValueAsGameplayTag(weatherSettings.MatchSettingsWeatherTypePropertyName, weatherType) && weatherType.IsValid())
         {
            TSoftClassPtr<ATATWeatherPreset> presetType = GetWeatherPresetClassForWeatherType(worldContext, weatherType);
            if (!presetType.IsNull())
            {
               return presetType;
            }
         }
      }
   }

   // Get the fallback weather type if needed.
   if (weatherSettings.FallbackWeatherType.IsValid())
   {
      TSoftClassPtr<ATATWeatherPreset> presetType = GetWeatherPresetClassForWeatherType(worldContext, weatherSettings.FallbackWeatherType);
      if (!presetType.IsNull())
      {
         UE_CLOG(logOrEnsureWhenReturningFallbacks, LogTATWeatherSettings, Warning, TEXT("Failed to get valid weather type from match settings - using fallback weather type '%s'"),
            *weatherSettings.FallbackWeatherType.ToString());
         return presetType;
      }
   }

   // In theory we should never get this far - we should have a weather type from match settings, and we should _always_ have a valid fallback weather type
   // that can be used to look up a level-appropriate preset class to use.
   // If we got this far, it's likely that either FallbackWeatherType isn't valid, or the data table entry for FallbackWeatherType has a null preset class.
   if (logOrEnsureWhenReturningFallbacks)
   {
      ensureMsgf(false, TEXT("Failed to get valid weather type from match settings. Also failed to get fallback weather type. Using fallback weather preset class '%s'"),
         *weatherSettings.FallbackWeatherPreset.ToString());
   }

   // If we got this far, we better have a valid fallback preset class
   ensureMsgf(!weatherSettings.FallbackWeatherPreset.IsNull(), TEXT("TATWeatherSettings.FallbackWeatherPreset is not set to a valid weather preset type!"));

   return weatherSettings.FallbackWeatherPreset;
}

// static
FGameplayTag UTATWeatherSettings::GetCurrentWeatherType(const UObject* worldContext, bool& returnedDefaultType)
{
   returnedDefaultType = false;

#if WITH_EDITOR
   // Check for an editor override. The user could have set an override for the preset class instead. Presets don't map directly to specific weather types
   // (multiple weather types could theoretically point to the same preset), so we simply don't have a way to handle that case.
   const UTATEditorSettings& editorSettings = UTATEditorSettings::Get();
   if (editorSettings.WeatherOverride == ETATEditorSettingsWeatherOverrideMode::OverrideWeatherType)
   {
      return editorSettings.WeatherTypeOverride;
   }
#endif
   
   const UTATWeatherSettings& weatherSettings = UTATWeatherSettings::Get();

   // Try getting the weather type from the game instance's copy of match settings.
   // In the normal match flow, this is generally where we expect to get the canonical value.
   UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull);
   if (world != nullptr && weatherSettings.MatchSettingsWeatherTypePropertyName != NAME_None)
   {
      if (UTATGameInstance* gameInstance = world->GetGameInstance<UTATGameInstance>())
      {
         FGameplayTag weatherType;
         if (gameInstance->GetMatchSettings().GetMatchSettingsValueAsGameplayTag(weatherSettings.MatchSettingsWeatherTypePropertyName, weatherType) && weatherType.IsValid())
         {
            return weatherType;
         }
      }
   }

   returnedDefaultType = true;
   return weatherSettings.FallbackWeatherType;
}

const FTATWeatherTypeInfo* UTATWeatherSettings::GetCurrentWeatherTypeInfo(const UObject* worldContext, bool* returnedDefault, const UDataTable* weatherDataTable) const
{
   bool isDefault = false;
   const FTATWeatherTypeInfo* weatherTypeInfo = FindWeatherTypeInfo(GetCurrentWeatherType(worldContext, isDefault), weatherDataTable);
   if (returnedDefault != nullptr)
   {
      *returnedDefault = isDefault;
   }
   return weatherTypeInfo;
}
