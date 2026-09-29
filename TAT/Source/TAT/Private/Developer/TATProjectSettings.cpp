// (c) 2020-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Developer/TATProjectSettings.h"

#include "Character/TATTeams.h"
#include "GameFramework/TATWorldSettings.h"
#include "Settings/TATMatchSettings.h"

// ue
#include "Engine/Engine.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATProjectSettings)

DEFINE_LOG_CATEGORY_STATIC(LogTATProjectSettings, Log, All);

namespace ProjectSettingsHelpers
{
   static FSoftObjectPath GetCanonicalPathForWorld(const UWorld* world)
   {
      FSoftObjectPath path = world->GetPathName();
#if WITH_EDITOR
      if (world->IsPlayInEditor())
      {
         path.SetPath(UWorld::RemovePIEPrefix(path.ToString()));
      }
#endif

      return path;
   }
}

const FTATMapSettings* FTATMapSettingsHandle::Get() const
{
   const UTATProjectSettings& settings = UTATProjectSettings::Get();
   return settings.Maps.IsValidIndex(MapIndex) ? &settings.Maps[MapIndex] : nullptr;
}

UTATProjectSettings::UTATProjectSettings()
{
   for (int idx = 0; idx < int(ETATTeamCharacterType::MAX); ++idx)
   {
      TeamAssignments.Add(ETATTeamCharacterType(idx), 0);
   }
}

FTATMapSettings UTATProjectSettings::BP_FindMapSettings(const FSoftObjectPath& map) const
{
   if (const FTATMapSettings* mapSettings = FindMapSettings(map))
   {
      return *mapSettings;
   }
   return FTATMapSettings();
}

const FTATMapSettings* UTATProjectSettings::FindMapSettings(const FSoftObjectPath& map) const
{
   for (const FTATMapSettings& mapSettings : Maps)
   {
      if (mapSettings.Map.ToSoftObjectPath() == map)
      {
         return &mapSettings;
      }
   }

   return nullptr;
}

const FTATMapSettings* UTATProjectSettings::FindMapSettings(const UWorld* world) const
{
   if (world == nullptr)
   {
      return nullptr;
   }
   return FindMapSettings(ProjectSettingsHelpers::GetCanonicalPathForWorld(world));
}

const FTATMapSettings* UTATProjectSettings::FindMapSettings(const FGameplayTag& mapTag) const
{
   return Maps.FindByPredicate([&mapTag](const FTATMapSettings& map) { return mapTag == map.MapTag; });
}

FTATMapSettingsHandle UTATProjectSettings::BP_GetMapSettingHandleForTag(const FGameplayTag& mapTag)
{
   return FTATMapSettingsHandle {
      .MapIndex = Get().Maps.IndexOfByPredicate([&mapTag](const FTATMapSettings& map) { return mapTag == map.MapTag; })
   };
}

FTATMapSettingsHandle UTATProjectSettings::BP_GetMapSettingHandleForPath(const FSoftObjectPath& mapPath)
{
   return FTATMapSettingsHandle {
      .MapIndex = Get().Maps.IndexOfByPredicate([&mapPath](const FTATMapSettings& map) { return mapPath == map.Map.ToSoftObjectPath(); })
   };
}

bool UTATProjectSettings::MapHandle_IsValid(const FTATMapSettingsHandle& mapHandle)
{
   return mapHandle.IsValid();
}

FGameplayTag UTATProjectSettings::MapHandle_GetMapTag(const FTATMapSettingsHandle& mapHandle)
{
   if (const FTATMapSettings* settings = mapHandle.Get())
   {
      return settings->MapTag;
   }
   return FGameplayTag();
}

FText UTATProjectSettings::MapHandle_GetDisplayName(const FTATMapSettingsHandle& mapHandle)
{
   if (const FTATMapSettings* settings = mapHandle.Get())
   {
      return settings->MapDisplayName;
   }
   return FText();
}

const FTATMapTypeSettings& UTATProjectSettings::GetMapTypeSettings(ETATMapType mapType) const
{
   check(static_cast<uint32>(mapType) < static_cast<uint32>(ETATMapType::MAX));
   return MapTypeSettings[static_cast<uint32>(mapType)];
}

const FTATMapTypeSettings* UTATProjectSettings::GetMapTypeSettingsForCurrentWorld(const UObject* worldContextObject) const
{
   if (UWorld* world = GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::ReturnNull))
   {
      if (ATATWorldSettings* worldSettings = Cast<ATATWorldSettings>(world->GetWorldSettings()))
      {
         return &GetMapTypeSettings(worldSettings->MapType);
      }
   }
   return nullptr;
}

const FTATMapTypeSettings& UTATProjectSettings::GetMapTypeSettingsForCurrentWorldChecked(const UObject* worldContextObject) const
{
   const FTATMapTypeSettings* settings = GetMapTypeSettingsForCurrentWorld(worldContextObject);
   check(settings != nullptr);
   return *settings;
}

// static
bool UTATProjectSettings::BP_GetMapTypeSettings_MapType(ETATMapType mapType, FTATMapTypeSettings& settings)
{
   if (static_cast<uint32>(mapType) < static_cast<uint32>(ETATMapType::MAX))
   {
      settings = UTATProjectSettings::Get().GetMapTypeSettings(mapType);
      return true;
   }
   settings = FTATMapTypeSettings{};
   return false;
}

// static
bool UTATProjectSettings::BP_GetMapTypeSettings_CurrentWorld(const UObject* worldContextObject, FTATMapTypeSettings& settings)
{
   if (const FTATMapTypeSettings* mapTypeSettings = UTATProjectSettings::Get().GetMapTypeSettingsForCurrentWorld(worldContextObject))
   {
      settings = *mapTypeSettings;
      return true;
   }
   settings = FTATMapTypeSettings{};
   return false;
}

TSubclassOf<UTATMatchSettingsBase> UTATProjectSettings::GetMatchSettingsClass() const
{
   if (!DefaultMatchSettingsClass.IsNull())
   {
      return DefaultMatchSettingsClass.LoadSynchronous();
   }
   return UTATMatchSettings::StaticClass();
}

FCollisionResponseContainer UTATProjectSettings::GetLyingDownCollisionMask() const
{
   // TODO: try to only do this once?
   FCollisionResponseContainer result;
   for (ECollisionChannel channel : CapsuleChannelsToIgnoreWhenDown)
   {
      result.SetResponse(channel, ECR_Ignore);
   }
   return result;
}

/* static */
uint8 UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType characterType)
{
   const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
   if (const uint8* teamAssignment = projectSettings.TeamAssignments.Find(characterType))
   {
      return *teamAssignment;
   }
   return static_cast<uint8>(characterType);
}

float UTATProjectSettings::GetCosineSneakAttackFacingHalfAngle() const
{
   // Using mutable is not in general thread-safe as const would imply, but this should only be called from the GT
   check(IsInGameThread());

   // Check if our cached value is still good
   if (_cachedSneakAttackFacingAngleWidth < 0 || _cachedSneakAttackFacingAngleWidth != SneakAttackFacingAngleWidth)
   {
      _cachedSneakAttackFacingAngleWidth = SneakAttackFacingAngleWidth;
      _cachedCosineSneakAttackFacingHalfAngle = FMath::Cos(FMath::DegreesToRadians(_cachedSneakAttackFacingAngleWidth / 2.0f));
   }

   return _cachedCosineSneakAttackFacingHalfAngle;
}

const float UTATProjectSettings::GetEndgameDurationForDifficulty(ETATDifficulty difficulty) const
{
   const int32 index = static_cast<int32>(difficulty);
   if (index >= 0 && index < UE_ARRAY_COUNT(EndgameDurationsByDifficulty))
   {
      return EndgameDurationsByDifficulty[index];
   }

   UE_LOG(LogTATProjectSettings, Warning, TEXT("Could not find a configured Endgame Duration for current difficulty (%s), defaulting to 20 minutes!"), *UEnum::GetValueAsString(difficulty));
   return 1200.0f;
}

void UTATProjectSettings::AppendEffectsToPreload(TArray<FSoftObjectPath>& outPathsToLoad) const
{
   outPathsToLoad.Add(RepairBreakableEffect.ToSoftObjectPath());
   // not an effect, but keep it alive
   outPathsToLoad.Add(DefaultCharacterMetadata.ToSoftObjectPath());
}

bool UTATProjectSettings::ShouldUseIndividualAttitudes()
{
   const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
   return projectSettings.UseIndividualAttitudes;
}

bool UTATProjectSettings::ShouldUseLightDetection()
{
   const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
   return projectSettings.UseLightDetection;
}
