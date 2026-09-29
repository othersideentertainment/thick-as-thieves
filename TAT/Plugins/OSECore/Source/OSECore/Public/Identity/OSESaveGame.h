// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"

// ose
#include "Identity/OSESaveGameSystem.h"

#include "OSESaveGame.generated.h"

class UOSESaveGameSystem;

//---------------------------------------------------------------------------------------
/// OSE Save Game
/// 
/// A save game data object, with 'save priority' functionality. A latent save can be
/// triggered which can happen at a later time, helping to avoid cert failure for
/// writing to disk too frequently (timer part of the save system). 
/// 
/// Uses `OSESaveGameSystem` to actually 'write-to-disk'.
//---------------------------------------------------------------------------------------
UCLASS()
class OSECORE_API UOSESaveGame : public USaveGame
{
   GENERATED_BODY()

public:

   /// Called on load or create, with save system reference.
   /// \see UOSESaveGameSystem::Load
   /// \see UOSESaveGameSystem::CreateNewSaveGame
   void SetSaveGameSystem(UOSESaveGameSystem* saveGameSystem);

protected:
   /// Save game system pointer (instantiates this).
   UPROPERTY(Transient)
   UOSESaveGameSystem* _saveGameSystem = nullptr;



   //--------------------------------------------------------------------------
   // Is Dirty Methods
protected:
   bool _isDirty = false;

public:
   /// Returns true if the save-data is marked dirty.
   bool IsDirty() const { return _isDirty; }

protected:
   /// Clears the 'is dirty' state. Called on write-to-disk
   /// by save system.
   void _ClearIsDirty() { _isDirty = false; }

   /// Save the game (write-to-disk), either immediate, or latent.
   /// Derived classes should call this.
   void _MarkDirty(EOSESavePriority priority);

   //--------------------------------------------------------------------------
   // Save Load Create Callbacks
public:
   /// On save-data created. Calls virtual `OnSaveDataCreated()`.
   void OnCreated();

   /// On save data loaded. Calls virtual `OnSaveDataLoaded()`.
   void OnLoaded();

   /// On save write-to-disk start (before)
   void OnBeforeSaveToDisk();

   /// On save data written-to-disk (after)
   void OnSavedToDisk();

protected:
   /// Virtual for new save-data created
   virtual void _OnSaveDataCreated() {}

   /// Virtual for loaded from disk
   virtual void _OnSaveDataLoaded() {}

   /// Virtual for save data written-to-disk (after)
   virtual void _OnSaveDataSavedToDisk() {}



   //--------------------------------------------------------------------------
   // Save Version Handling
public:
   /// Gets the clear version for current saves [constant, compiled-in]
   virtual int GetSaveCurrentClearVersion() const { unimplemented(); return 0; }
   
   /// Gets the current version of the save-data [constant, compiled-in]
   virtual int GetSaveCurrentVersion() const { unimplemented(); return 0; }

   /// Gets the save version of this save data object [deserialized]
   int GetSaveVersion() const { return _saveVersion; }
   
   /// Gets the clear version of this save data object [deserialized]
   int GetClearVersion() const { return _clearVersion; }

protected:
   UPROPERTY()
   int _saveVersion = 0;

   UPROPERTY()
   int _clearVersion = 0;

   /// Correct older version save-data.
   /// Called to update old saves to newest version (unless the
   /// save is below minimum version and cleared).
   /// \see UOSESaveGame::_GetSaveMinimumVersion
   virtual void _FixupSaveVersion() {}
};
