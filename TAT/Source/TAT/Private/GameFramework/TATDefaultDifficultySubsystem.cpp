// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATDefaultDifficultySubsystem.h"

// tat
#include "SaveGame/TATSaveGame.h"

// ose
#include "TATGameInstance.h"
#include "Identity/OSESaveGameSystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDefaultDifficultySubsystem)

class UOSESaveGameSystem;

bool UTATDefaultDifficultySubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   // N/A on servers
   if(CastChecked<UGameInstance>(outer)->IsDedicatedServerInstance())
   {
      return false;
   }
   
   return Super::ShouldCreateSubsystem(outer);
}

void UTATDefaultDifficultySubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

   // waiting a frame to make sure UOSESaveGameSystem is initialized
   GetGameInstance()->GetTimerManager().SetTimerForNextTick(this, &UTATDefaultDifficultySubsystem::_WaitForSave);
}

void UTATDefaultDifficultySubsystem::_WaitForSave()
{
   UOSESaveGameSystem* saveGameSystem = UOSESaveGameSystem::Get(this);
   check(saveGameSystem);
   if (saveGameSystem->HasLoadedSaveData())
   {
      _InitializeFromSave();
   }
   else
   {
      saveGameSystem->OnSaveDataStateChanged.AddUniqueDynamic(this, &ThisClass::_OnSaveDataStateChanged);
   }
}

void UTATDefaultDifficultySubsystem::_OnSaveDataStateChanged(EOSESaveDataState state)
{
   if(state == EOSESaveDataState::HasSaveData)
   {
      _InitializeFromSave();
   }
}

void UTATDefaultDifficultySubsystem::_InitializeFromSave()
{
   UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
   check(saveGame);

   if (saveGame->GetSelectedDifficulty().IsSet())
   {
      _SetGameInstanceDifficulty(*saveGame->GetSelectedDifficulty());
   }

   if (UTATGameInstance* gameInstance = CastChecked<UTATGameInstance>(GetGameInstance()))
   {
      gameInstance->OnMapNodeSettingsUpdated.AddUniqueDynamic(this, &ThisClass::_OnMapNodeSettingsChanged);
   }

   _RefreshDifficultyUnlock(true);
   saveGame->OnUnlocksChanged.AddUniqueDynamic(this, &ThisClass::_OnUnlocksChanged);
}

void UTATDefaultDifficultySubsystem::_OnUnlocksChanged()
{
   _RefreshDifficultyUnlock(false);
}

void UTATDefaultDifficultySubsystem::_RefreshDifficultyUnlock(bool isInitial)
{
   UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
   if(!ensure(saveGame))
   {
      return;
   }

   const bool newDifficultyUnlock = saveGame->HasUnlockedContent(TAG_Difficulty_Normal);
   if(newDifficultyUnlock == _hadUnlockedNormal)
   {
      return;
   }

   _hadUnlockedNormal = newDifficultyUnlock;
   // Only change difficulty to normal if: Just-unlocked, or difficulty never selected
   if(newDifficultyUnlock && (!isInitial || !saveGame->GetSelectedDifficulty().IsSet()))
   {
      _SetGameInstanceDifficulty(ETATDifficulty::Normal);
   }
}

void UTATDefaultDifficultySubsystem::_OnMapNodeSettingsChanged(const FTATMapNodeSettings& newMapNodeSettings)
{
   UTATSaveGame* saveGame = UTATSaveGame::GetTATSaveGame(this);
   if (!ensure(saveGame))
   {
      return;
   }

   // Only set saved difficulty if already set or changing from implicit default
   // Avoids setting it when not intended
   if (saveGame->GetSelectedDifficulty().IsSet() || newMapNodeSettings.Difficulty != ETATDifficulty::Easy)
   {
      saveGame->SetSelectedDifficulty(newMapNodeSettings.Difficulty);
   }
}

void UTATDefaultDifficultySubsystem::_SetGameInstanceDifficulty(ETATDifficulty newDifficulty)
{
   UTATGameInstance* gameInstance = CastChecked<UTATGameInstance>(GetGameInstance());
   if (gameInstance && gameInstance->GetMapNodeSettings().Difficulty != newDifficulty)
   {
      FTATMapNodeSettings newSettings = gameInstance->GetMapNodeSettings();
      newSettings.Difficulty = newDifficulty;
      gameInstance->SetMapNodeSettings(newSettings);
   }
}

