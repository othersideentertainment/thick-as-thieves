// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Templates/UniquePtr.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/SaveGame.h"
#include "OnlineSubsystem.h"
#include "Identity/OSEPlatformIdentity.h"
#include "SaveGameSystem.h"

// ose
#include "Identity/OSEPlatformIdentity.h"

#include "OSESaveGameSystem.generated.h"

class UOSESaveGame;
class UGameInstance;
class ISaveGameSystem;

/// States for save game data state machine
UENUM(BlueprintType)
enum class EOSESaveDataState : uint8
{
   NoSaveData,       ///< No save game data
   HasSaveData,      ///< Has save game data
   PreUnload,        ///< About to unload (log-out, etc.)
};

/// States for save IO operations. Also used in 'IO fail' event.
UENUM(BlueprintType)
enum class EOSESaveIOState : uint8
{
   NoOperation,      ///< Not loading/saving
   Loading,          ///< Loading (no save game data)
   Creating,         ///< Creating new save game
   Saving,           ///< Saving to disk
   Deleting,         ///< Deleting save game
};

/// Save timer/priority enum. Used for 'save timer' immediate/latent-saves.
UENUM(BlueprintType)
enum class EOSESavePriority : uint8
{
   None,             ///< No save queued
   HighPriority,     ///< High-priority (1-frame delay)
   LowPriority,      ///< Low-priority (latent-save, triggers timer)
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSESaveGameDataStateChangedDelegate, EOSESaveDataState, state);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSESaveIOStateChange, EOSESaveIOState, state);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSESaveIOFailure, EOSESaveIOState, failureState);
DECLARE_DELEGATE_ThreeParams(FOSESaveExistsDelegate, const FString&, int32, ISaveGameSystem::ESaveExistsResult);


//---------------------------------------------------------------------------------------
/// OSE Save Game System
///
/// A platform-generic system which handles retrieval, reading, and writing of a player's 
/// save game data with a save game data state system and events on state change. Handles
/// the lifetime of the save game data object.
///
/// Editor only: attaches to PIE 'begin play' event to load save game data on all maps.
/// 
/// Used by OSEIdentityMgr.
//---------------------------------------------------------------------------------------
UCLASS()
class OSECORE_API UOSESaveGameSystem : public UObject
{
   GENERATED_BODY()

public:

   /// On startup (OSE Identity Manger subsystem, with reference to GameInstance)
   bool Initialize(UGameInstance* owningGameInstance);

   /// On shutdown (OSE Identity Manger subsystem)
   void Deinitialize();

   UFUNCTION(BlueprintCallable, Category = "Save|OSE")
   static UOSESaveGameSystem* Get(const UObject* contextObj);

private:
   /// Reference to the current GameInstance. 
   /// \see UOSEPlatformMgr::Initialize
   UPROPERTY(Transient)
   UGameInstance* _gameInstance = nullptr;

   /// Reference to the platform's save game system.
   /// Facilitates loading/saving of player's save data.
   ISaveGameSystem* _saveGameSystem = nullptr;

   /// Save game data object (actual save data).
   UPROPERTY(Transient)
   UOSESaveGame* _saveData = nullptr;

   /// The 'class' of the save game object (used on create/load)
   TSubclassOf<UOSESaveGame> _saveGameClass;

   /// Set the save data object (sets appropriate save data state).
   void _SetSaveData(UOSESaveGame* newSaveData);

public:
   /// Get the save game data.
   UFUNCTION(BlueprintCallable, Category="Save|OSE")
   UOSESaveGame* GetSaveData() const { return _saveData; }



   //--------------------------------------------------------------------------
   // State machine
private:

   /// Current save game data state for state machine. Initial: 'NoSaveData'.
   EOSESaveDataState _saveDataState = EOSESaveDataState::NoSaveData;

   /// Change save game data state (internal). 
   void _SetSaveDataState(EOSESaveDataState newState);

   /// Current IO operation state for state machine. Initial: 'NoOperation'.
   EOSESaveIOState _saveIOState = EOSESaveIOState::NoOperation;

   /// Change save IO state (internal). 
   void _SetIOState(EOSESaveIOState newState);

public:
   /// Returns the current save data state (state machine).
   UFUNCTION(BlueprintCallable, Category = "Save|OSE")
   EOSESaveDataState GetSaveDataState() const { return _saveDataState; }

   /// Returns the current save data state (state machine).
   UFUNCTION(BlueprintCallable, Category = "Save|OSE")
   EOSESaveIOState GetSaveIOState() const { return _saveIOState; }

   /// Returns true if save game data is valid (loaded).
   UFUNCTION(BlueprintCallable, Category = "Save|OSE")
   bool HasLoadedSaveData() const { return _saveData != nullptr; }

   /// Returns true if a save data exists already (on-disk)
   UFUNCTION(BlueprintCallable, Category = "Save|OSE")
   bool HasOnDiskSaveData() const;

   /// Save Game data state changed. Raised on save game data 
   /// loaded/unloaded. With enum `newState` param. 
   /// \see UOSESaveGameSystem::_SetSaveGameDataState
   UPROPERTY(BlueprintAssignable, category = "Identity|OSE", meta=(Tooltip="'Loading': no save data\n'Saving': has save data"))
   FOSESaveGameDataStateChangedDelegate OnSaveDataStateChanged;

   /// Save data IO operation (save, load) state change.
   /// \see UOSEPlatformIdentity::_SetIOState
   UPROPERTY(BlueprintAssignable, category = "Identity|OSE", meta = (Tooltip = "Raised when saving or loading state changes"))
   FOSESaveIOStateChange OnSaveIOStateChange;

   /// Save data IO operation failure.
   /// \see UOSEPlatformIdentity::_SetIOState
   UPROPERTY(BlueprintAssignable, category = "Identity|OSE", meta = (Tooltip = "Raised on IO (load/save) fail"))
   FOSESaveIOFailure OnSaveIOFail;



   //--------------------------------------------------------------------------
   ///@{
   /// Has methods
private:
   /// Callback delegate for checking if a save exists (calls `_SaveExistsCallback()`).
   FOSESaveExistsDelegate _saveExistsAsyncCallbackDelegate;

   /// Callback on result from async does save exist
   void _SaveExistsCallback(const FString&, int32, ISaveGameSystem::ESaveExistsResult);
   ///@}


   //--------------------------------------------------------------------------
   // Load methods
public:

   /// Load the player's save data (blocking).
   /// Returns true for successful load.
   /// If no on-disk save data, calls `CreateNewSaveGame`.
   /// Sets state to either 'HasSaveData' or 'NoSaveData'.
   /// \returns True if the save data was loaded successfully.
   bool Load();

   /// Begin loading the player's save data asynchronously. 
   /// Sets IO state to 'Creating' while async task on-going.
   /// If no on-disk save data, calls `CreateNewSaveGame`.
   /// Raises `OnSaveDataStateChanged` on success. On fail, 
   /// raises `OnSaveIOFail`.
   void LoadAsync();

private:
   /// Callback delegate for load async (calls `_LoadAsyncCallback()`).
   FAsyncLoadGameFromSlotDelegate _loadAsyncCallbackDelegate;

   /// Callback on result from async 'load save game'.
   /// \see `UOSESaveGameSystem::LoadSaveDataAsync`
   void _LoadAsyncCallback(const FString&, const int32, USaveGame*);

   UOSESaveGame* _ResolveSaveDataClearVersion(UOSESaveGame* saveGame);


   //--------------------------------------------------------------------------
   // Save methods
public:

   /// Save player's current data to disk (blocking).
   /// Returns true for successful save.
   /// \returns True if the save data was saved successfully.
   bool Save();

   /// Begin saving the player's save data asynchronously.
   /// Sets IO state to 'Saving' while async task on-going.
   /// Raises `OnSaveDataStateChanged` on success. On fail, 
   /// raises `OnSaveIOFail`.
   void SaveAsync();

private:
   /// Callback delegate for Save async (calls `_SaveAsyncCallback()`).
   FAsyncSaveGameToSlotDelegate _saveAsyncCallbackDelegate;

   /// Callback on result from async 'Save save game'.
   /// \see `UOSESaveGameSystem::SaveSaveDataAsync`
   void _SaveAsyncCallback(const FString& slotName, const int32 slotUserIndex, bool success);



   //--------------------------------------------------------------------------
   // Create methods
private:

   /// Create new save game data (writes to disk).
   /// Spawns save game UObject of class from project settings.
   /// Use `GetSaveData()` after to retrieve the save object.
   /// Raises `OnSaveDataStateChanged` on success.
   /// Raises `OnSaveIOStateChanged` (may raise `OnSaveIOFail`).
   /// \see UOSEProjectSettings::SaveGameClass
   bool CreateNewSaveGame();



   //--------------------------------------------------------------------------
   // Delete methods
public:

   /// Delete player's save data (blocking). 
   /// Deletes main save data (no per-identity save files yet).
   /// Sets save data state to `NoSaveData`. On fail, raises
   /// `OnSaveIOFail`.
   /// \returns True if deleted successfully.
   bool Delete();


   //--------------------------------------------------------------------------
   // Internal (private)
private:

   /// Utility to return the OSE Platform Identity object.
   UOSEPlatformIdentity* _GetPlatformIdentity() const;

   /// Event handler on user 'logging out'.
   /// Sets save data state to 'PreUnload'.
   /// \param loginDomains The login domains to be cleaned up. 
   /// \see IOnlineIdentity::OnLoginFlowLogout
   void _OnLoginFlowLogout(const TArray<FString>& loginDomains);
   FDelegateHandle _loginFlowLogoutHandle = FDelegateHandle();

   /// Event handler for 'login state changed' from OSEPlatformIdentity. 
   /// If has data, but new status is `NotLoggedIn`, clears save data and 
   /// sets state `NoSaveData`
   /// \see UOSEPlatformIdentity::OnLoginStatusChanged
   UFUNCTION()
   void _OnLoginStatusChanged(EOSEPlatformLoginState newState);

   /// The Game's save slot name (from project/game config).
   FString _saveSlotName;

   /// Returns true if ready to save/load save game data (internal). 
   /// Checks for on-going IO operation, 'save game system', and player's 
   /// identity/uniqueNetId are valid.
   bool _IsReadyIO() const;



   //--------------------------------------------------------------------------
   // Save timer system
   // 
   // A system which guards against 'write-to-disk' happening too frequently.
   // The save timer system will handle `OSESaveGame` priority changes, and 
   // queue immediate/latent saves accordingly.

public:
   /// Raised when the save-data is marked 'dirty'. Conditionally updates
   /// the save-timer with an earlier time.
   void OnSaveDataMarkedDirty(EOSESavePriority priority);

private:
   /// Current save priority save-timer is based on.
   /// \see UOSESaveGameSystem::_UpdateSaveTimerInterval
   /// \see UOSESaveGameSystem::_GetSaveTimerInterval
   EOSESavePriority _saveTimerPriority = EOSESavePriority::None;

   /// Handle for current save-timer.
   FTimerHandle _saveTimerHandle = FTimerHandle();

   /// Current interval of the save-timer.
   float _saveTimerInterval = 0.0f;

   /// Expected end-time for the current save-timer
   FDateTime _saveTimerEnd = FDateTime();

   /// True if a new save-timer ticks during a save IO operation.
   bool _pendingSave = false;

   /// Returns the interval of the save-timer for a priority. 
   static float _GetSaveTimerInterval(EOSESavePriority priority);

   /// Set the save-timer interval (clears old timers).
   void _SetSaveTimerInterval(float seconds);

   /// Clear on-going timer (reset timer handle).
   void _ClearSaveTimer();

   /// Raised when a scheduled save timer expire (write-to-disk).
   UFUNCTION()
   void _OnSaveTimerExpire();

#if WITH_EDITOR
   /// Raised immediately after a map is loaded (unbinds itself before returning)
   void _OnPostLoadMapWithWorld(UWorld* loadedWorld);
#endif // WITH_EDITOR
};
