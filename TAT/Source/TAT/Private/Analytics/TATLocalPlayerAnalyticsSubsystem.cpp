// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Analytics/TATLocalPlayerAnalyticsSubsystem.h"

// tat
#include "TATGameInstance.h"
#include "Analytics/TATAnalyticsManager.h"
#include "Developer/TATProjectSettings.h"
#include "Loot/TATLootUtils.h"
#include "SaveGame/TATCharacterDataContext.h"
#include "SaveGame/TATSaveGame.h"
#include "Settings/TATMatchSettings.h"

// ose 
#include "Player/OSEPlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLocalPlayerAnalyticsSubsystem)

DEFINE_LOG_CATEGORY_STATIC(LogTATLocalPlayerAnalyticsSubsystem, Warning, All);

namespace TATLocalPlayerAnalyticsSubsystemHelpers
{
   FString _ConvertBoolToString(const bool& val)
   {
      return FString(val ? TEXT("True") : TEXT("False"));
   }
   FString _ConvertDoubleToString(const double& val)
   {
      return FString::SanitizeFloat(val);
   }
   FString _ConvertIntToString(const int& val)
   {
      return FString::FromInt(val);
   }
   FString _ConvertGameplayTagToSafeName(const FString& name)
   {
      return name.Replace(TEXT("."), TEXT("_"));
   }
}

void UTATLocalPlayerAnalyticsSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);
   
   // This is initialized for the lifetime of the local player, which on this title _should_ be almost equal to the length
   // of time that the game has been running for.
   _TimeBooted = FPlatformTime::Seconds();
}

void UTATLocalPlayerAnalyticsSubsystem::HandleMatchStart() const
{
   FTATAnalyticsCustomFields analyticsFields;
   _SetMatchDetails(analyticsFields);
   _SetPlayerDetails(analyticsFields);
   if (UTATAnalyticsManager* analyticsManager = GetWorld()->GetGameInstance()->GetSubsystem<UTATAnalyticsManager>())
   {
      analyticsManager->OnDesignEventWithCustomFields(TEXT("MatchStart"), analyticsFields);
   }
}

void UTATLocalPlayerAnalyticsSubsystem::HandleMatchEnd(const FMatchPersistentData& data) const
{
   FTATAnalyticsCustomFields analyticsFields;
   _SetMatchDetails(analyticsFields);
   _SetPlayerDetails(analyticsFields);
   
   analyticsFields.Set(TEXT("ExperienceGained"), TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertIntToString(data.XPGained));
   analyticsFields.Set(TEXT("CompletionStatus"), TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertBoolToString(data.CompletionState == EMatchCompletionState::Escaped));
   
   // If iterating the loot containers gets expensive, might want to refactor this to do a single iteration of the array and pull out the relevant data into a map.
   analyticsFields.Set(TEXT("LootValue_Major_Carried"), TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertIntToString(UTATLootUtils::CountValueLootOfType(GetWorld(), data.CarriedLoot, ETATLootType::MajorLoot)));
   analyticsFields.Set(TEXT("LootValue_Major_Stashed"), TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertIntToString(UTATLootUtils::CountValueLootOfType(GetWorld(), data.StashedLoot, ETATLootType::MajorLoot)));
   analyticsFields.Set(TEXT("LootValue_Major_AllyCarried"), TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertIntToString(UTATLootUtils::CountValueLootOfType(GetWorld(), data.AllyCarriedLoot, ETATLootType::MajorLoot)));
   analyticsFields.Set(TEXT("LootValue_Minor_Carried"), TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertIntToString(UTATLootUtils::CountValueLootOfType(GetWorld(), data.CarriedLoot, ETATLootType::MinorLoot)));
   analyticsFields.Set(TEXT("LootValue_Minor_Stashed"), TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertIntToString(UTATLootUtils::CountValueLootOfType(GetWorld(), data.StashedLoot, ETATLootType::MinorLoot)));
   analyticsFields.Set(TEXT("LootValue_Minor_AllyCarried"), TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertIntToString(UTATLootUtils::CountValueLootOfType(GetWorld(), data.AllyCarriedLoot, ETATLootType::MinorLoot)));
   
   if (APlayerController* playerController = GetLocalPlayer()->GetPlayerController(GetWorld()))
   {
      if (AOSEPlayerState* osePlayerState = playerController->GetPlayerState<AOSEPlayerState>())
      {
         const FOSEPlayerStats& playerStats = osePlayerState->GetPlayerStats();
         for (const FOSEPlayerStat& stat : playerStats.Stats)
         {
            analyticsFields.Set(
               TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertGameplayTagToSafeName(stat.Tag.ToString()),
               TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertIntToString(stat.IntValue)
               );
         }
      }
   }
   
   if (UTATAnalyticsManager* analyticsManager = GetWorld()->GetGameInstance()->GetSubsystem<UTATAnalyticsManager>())
   {
      analyticsManager->OnDesignEventWithCustomFields(TEXT("MatchEnd"), analyticsFields);
   }
}

void UTATLocalPlayerAnalyticsSubsystem::_SetPlayerDetails(FTATAnalyticsCustomFields& analyticsFields) const
{
   if (UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this))
   {
      const FTATCharacterSaveId selectedCharacter = saveGame->GetSelectedCharacter();
      analyticsFields.Set(TEXT("Character"), selectedCharacter.ToString());

      analyticsFields.Set(TEXT("PlayerLevel"), TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertIntToString(saveGame->GetPlayerProgression().XP.Level));

      const FTATCharacterDataContext dataContext(saveGame, selectedCharacter);
      const FTATCharacterLoadout characterLoadout = dataContext.SaveGame->GetCharacterLoadout(dataContext.SaveId, ETATLoadoutType::Tool);
      for (const FTATCharacterLoadoutEntry& entry : characterLoadout.Entries)
      {
         analyticsFields.Set(
            TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertGameplayTagToSafeName(entry.LoadoutTag.ToString()),
            TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertBoolToString(true));
      }
   }
   else
   {
      UE_LOG(LogTATLocalPlayerAnalyticsSubsystem, Error, TEXT("Unable to find save game"));
   }
}

void UTATLocalPlayerAnalyticsSubsystem::_SetMatchDetails(FTATAnalyticsCustomFields& analyticsFields) const
{
   const UTATGameInstance& tatGameInstance = UTATGameInstance::Get(this);
   const UTATMatchSettings* tatMatchSettings = Cast<UTATMatchSettings>(&tatGameInstance.GetMatchSettings());
   if (tatMatchSettings == nullptr)
      return;
   
   analyticsFields.Set(TEXT("Difficulty"), UEnum::GetValueOrBitfieldAsString(TATDifficulty::GetDifficultyForMatch(GetWorld())));
   analyticsFields.Set(TEXT("Mission"), tatMatchSettings->Mission.ToString());
   
   const UTATProjectSettings& projectSettings = UTATProjectSettings::Get();
   if (const FTATMapSettings* mapSettings = projectSettings.FindMapSettings(GetWorld()))
   {
      analyticsFields.Set(TEXT("Map"), mapSettings->MapTag.ToString());
   }
   else
   {
      analyticsFields.Set(TEXT("Map"), FString(TEXT("NOTFOUND")));
      UE_LOG(LogTATLocalPlayerAnalyticsSubsystem, Error, TEXT("Unable to find map settings for current world"));
   }
   // Note - set don't do listen servers currently, but if we do this check won't be enough
   const bool isOnline = GetWorld()->GetNetMode() != NM_Standalone;
   analyticsFields.Set(TEXT("IsOnline"), TATLocalPlayerAnalyticsSubsystemHelpers::_ConvertBoolToString(isOnline));
   analyticsFields.Set(TEXT("TimeRunning"), FPlatformTime::Seconds() - _TimeBooted);
}
