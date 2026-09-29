// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Identity/OSESaveGame.h"

// ose
#include "OSEGameInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESaveGame)

DEFINE_LOG_CATEGORY_STATIC(LogOSESaveGame, Log, All);

void UOSESaveGame::SetSaveGameSystem(UOSESaveGameSystem* saveGameSystem)
{
   _saveGameSystem = saveGameSystem;
}

void UOSESaveGame::OnCreated()
{
   // Set current and minimum save-versions
   _saveVersion = GetSaveCurrentVersion();
   _clearVersion = GetSaveCurrentClearVersion();

   // Call virtual
   _OnSaveDataCreated();
}

void UOSESaveGame::OnLoaded()
{
   // Update to newest save (if applicable)
   _FixupSaveVersion();
   _saveVersion = GetSaveCurrentVersion();
   _clearVersion = GetSaveCurrentClearVersion();

   // Call virtual
   _OnSaveDataLoaded();
}

void UOSESaveGame::OnBeforeSaveToDisk()
{
   _ClearIsDirty();
}

void UOSESaveGame::OnSavedToDisk()
{
   _OnSaveDataSavedToDisk();
}

void UOSESaveGame::_MarkDirty(EOSESavePriority priority)
{
   // Save the game (write-to-disk), either immediate, or latent.
   if (priority == EOSESavePriority::None)
   {
      return;
   }
   _isDirty = true;
   _saveGameSystem->OnSaveDataMarkedDirty(priority);
}

