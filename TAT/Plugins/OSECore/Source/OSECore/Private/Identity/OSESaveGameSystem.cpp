// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Identity/OSESaveGameSystem.h"

//ue4
#include "Async/Async.h"
#include "Engine.h"
#include "GameMapsSettings.h"
#include "OnlineSubsystem.h"
#include "PlatformFeatures.h"
#include "SaveGameSystem.h"

// ue4 editor
#if WITH_EDITOR
#include "Editor.h"
#endif

// ose
#include "OSEProjectSettings.h"
#include "Identity/OSESaveGame.h"
#include "Identity/OSEIdentityMgr.h"
#include "Identity/OSEPlatformIdentity.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESaveGameSystem)

static const int kDefaultSlotIndex = 0;

DEFINE_LOG_CATEGORY_STATIC(LogOSESaveGameSystem, Log, All);

bool UOSESaveGameSystem::Initialize(UGameInstance* owningGameInstance)
{
   _gameInstance = owningGameInstance;
   check(_gameInstance);

   const UOSEProjectSettings& projectSettings = UOSEProjectSettings::Get();

   // Cache 'save slot name' (read from project settings/config)
   _saveSlotName = projectSettings.SaveSlotName;

   // Save game class
   // If this check breaks, the save game class (derived from `UOSESaveGame`)
   // in OSE Project Settings has not been specified (required).
   if (!projectSettings.SaveGameClass.IsNull())
      _saveGameClass = projectSettings.SaveGameClass.LoadSynchronous();

   // don't init if not configured on this project
   if (!_saveGameClass || _saveSlotName.IsEmpty())
      return false;

   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("Save game class: %s"), *_saveGameClass->GetName());
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("Save game slot: %s"), *_saveSlotName);

#if WITH_EDITOR
   // in editor let's save each PIE window to a different slot name so each window can have it's own save...
   if (_gameInstance)
   {
      if (_gameInstance->GetWorld()->IsPlayInEditor())
      {
         if (const FWorldContext* context = _gameInstance->GetWorldContext())
         {
            _saveSlotName = FString::Printf(TEXT("%s_%d"), *_saveSlotName, context->PIEInstance);
         }
      }
   }
#endif // WITH_EDITOR
   
   _saveGameSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
   check(_saveGameSystem);

   // Create 'async save/load' delegates (callback)
   _saveExistsAsyncCallbackDelegate = FOSESaveExistsDelegate::CreateUObject(this, &UOSESaveGameSystem::_SaveExistsCallback);
   _loadAsyncCallbackDelegate = FAsyncLoadGameFromSlotDelegate::CreateUObject(this, &UOSESaveGameSystem::_LoadAsyncCallback);
   _saveAsyncCallbackDelegate = FAsyncSaveGameToSlotDelegate::CreateUObject(this, &UOSESaveGameSystem::_SaveAsyncCallback);

   // Bind 'about to logout' (identity)
   IOnlineSubsystem* subsystem = IOnlineSubsystem::Get();
   check(subsystem != nullptr);
   IOnlineIdentityPtr identity = subsystem->GetIdentityInterface();
   check(identity.IsValid());
   _loginFlowLogoutHandle = identity->AddOnLoginFlowLogoutDelegate_Handle(FOnLoginFlowLogoutDelegate::CreateUObject(this, &UOSESaveGameSystem::_OnLoginFlowLogout));

   // Bind 'login state change' (from `OSEPlatformIdentity`). 
   // On log-out, clear save data.
   _GetPlatformIdentity()->OnLoginStateChange.AddDynamic(this, &UOSESaveGameSystem::_OnLoginStatusChanged);

   // EDITOR: Auto-load on PIE begin
#if WITH_EDITOR
   UWorld* world = _gameInstance->GetWorld();
   if (world != nullptr && world->IsPlayInEditor())
   {
      Load();
   }
   else if(!_gameInstance->IsDedicatedServerInstance())
   {
      // Bind to initial map load (so we can force a save data load if bypassing bootload sequence)
      // NOTE: due to the way maps are loaded for PIE we can't rely on this delegate for PIE sessions; the first map-load callback is skipped
      FCoreUObjectDelegates::PostLoadMapWithWorld.AddUObject(this, &UOSESaveGameSystem::_OnPostLoadMapWithWorld);
   }
#endif // WITH_EDITOR

   return true;
}

void UOSESaveGameSystem::Deinitialize()
{ 
   // Unbind 'logging out' (identity)
   if (_loginFlowLogoutHandle.IsValid())
   {
      IOnlineSubsystem* subsystem = IOnlineSubsystem::Get();
      IOnlineIdentityPtr identity = subsystem->GetIdentityInterface();
      identity->OnLoginFlowLogoutDelegates.Remove(_loginFlowLogoutHandle);
   }

   // Unbind 'logged out' (from `OSEPlatformIdentity`). 
   _GetPlatformIdentity()->OnLoginStateChange.RemoveDynamic(this, &UOSESaveGameSystem::_OnLoginStatusChanged);

   // Save Timer: Clear on-going timer
   if (_saveTimerHandle.IsValid())
   {
      _gameInstance->GetTimerManager().ClearTimer(_saveTimerHandle);
      _saveTimerHandle = FTimerHandle();
   }

   auto shouldSaveOnExitIfDirty = [this]() {
      // EDITOR: Auto-save on PIE end
#if WITH_EDITOR
         UWorld* world = _gameInstance->GetWorld();
         if (world != nullptr && world->IsPlayInEditor())
         {
            return true;
         }
#endif

         return UOSEProjectSettings::Get().ShouldSaveOnExit;
      };

   if (_saveData != nullptr && _saveData->IsDirty() && shouldSaveOnExitIfDirty())
   {
      Save();
   }
}

UOSESaveGameSystem* UOSESaveGameSystem::Get(const UObject* contextObj)
{
   check(contextObj);
   UWorld* world = contextObj->GetWorld();
   check(world);
   UGameInstance* gameInstance = world->GetGameInstance();
   check(gameInstance);
   UOSEIdentityMgr* identityMgr = gameInstance->GetSubsystem<UOSEIdentityMgr>();
   check(identityMgr);
   return identityMgr->GetSaveGameSystem();
}

void UOSESaveGameSystem::_SetSaveData(UOSESaveGame* newSaveData)
{
   _saveData = newSaveData;
   _SetSaveDataState(_saveData != nullptr ? EOSESaveDataState::HasSaveData : EOSESaveDataState::NoSaveData);

   // Clear any previous save-timer
   _ClearSaveTimer();

   if (_saveData != nullptr)
   {
      _saveData->SetSaveGameSystem(this);

      // Start up repeating timer (check 'if dirty', save)
      float saveInterval = _GetSaveTimerInterval(EOSESavePriority::None);
      _gameInstance->GetTimerManager().SetTimer(_saveTimerHandle, this, &UOSESaveGameSystem::_OnSaveTimerExpire, saveInterval, true);
   }
}

bool UOSESaveGameSystem::HasOnDiskSaveData() const
{
   // Returns true if a save data exists already (on-disk)
   return UGameplayStatics::DoesSaveGameExist(_saveSlotName, 0);
}



//--------------------------------------------------------------------------
// State machine

void UOSESaveGameSystem::_SetSaveDataState(EOSESaveDataState newState)
{
   // Change save game data state (internal).
   
   // Already in 'new state'? Skip ...
   if (_saveDataState == newState)
      return;

   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("_SetSaveDataState: entering '%s' (leaving '%s')"),
      *UEnum::GetDisplayValueAsText(newState).ToString(), *UEnum::GetDisplayValueAsText(_saveDataState).ToString());

   _saveDataState = newState;
   OnSaveDataStateChanged.Broadcast(newState);

   // If the new state is `PreUnload` (on user about to sign-out), then
   // write unsaved data to disk
   if (newState == EOSESaveDataState::PreUnload)
   {
      // If in 'pre-unload', save data should be valid
      check(_saveData != nullptr);

      if (_saveData->IsDirty())
      {
         Save();
      }
   }
}

void UOSESaveGameSystem::_SetIOState(EOSESaveIOState newState)
{
   // Change save IO state (internal).
   
   // Already in 'new state'? Skip ...
   if (_saveIOState == newState)
   {
      return;
   }

   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("_SetIOState: entering '%s' (leaving '%s')"),
      *UEnum::GetDisplayValueAsText(newState).ToString(), *UEnum::GetDisplayValueAsText(_saveIOState).ToString());

   _saveIOState = newState;
   OnSaveIOStateChange.Broadcast(newState);
}



//--------------------------------------------------------------------------
// Load methods

bool UOSESaveGameSystem::Load()
{
   // Load the player's save data (blocking).
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("Load()"));

   // Has SaveGameSystem and Identity?
   if (!_IsReadyIO())
   {
      OnSaveIOFail.Broadcast(EOSESaveIOState::Loading);
      return false;
   }

   // Clear save game data (current)
   _SetSaveData(nullptr);

   // Create new save game?
   if (!HasOnDiskSaveData())
   {
      return CreateNewSaveGame();
   }

   // Load
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("Load(): Loading slot %s [index=%i]..."), *_saveSlotName, 0);
   _SetIOState(EOSESaveIOState::Loading);
   UOSESaveGame* newSaveData = Cast<UOSESaveGame>(UGameplayStatics::LoadGameFromSlot(_saveSlotName, 0));
   newSaveData = _ResolveSaveDataClearVersion(newSaveData);

   if (newSaveData != nullptr)
   {
      newSaveData->SetSaveGameSystem(this);
      newSaveData->OnLoaded();
   }
   _SetSaveData(newSaveData);
   _SetIOState(EOSESaveIOState::NoOperation);

   // Fail
   if (_saveDataState != EOSESaveDataState::HasSaveData)
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("Load(): Load failed (data is null)"));
      OnSaveIOFail.Broadcast(EOSESaveIOState::Loading);
      return false;
   }

   // Success
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("Load(): Load success"));
   return true;
}

/// Implement the missing wrapper method that should be in UGameplayStatistics
void UGameplayStatics_DoesSaveGameExistAsync(const FString& SlotName, const int32 UserIndex, FOSESaveExistsDelegate SaveExistsDelegate)
{
   ISaveGameSystem* SaveSystem = IPlatformFeaturesModule::Get().GetSaveGameSystem();
   if (SaveSystem && (SlotName.Len() > 0))
   {
      FPlatformUserId PlatformUserId = FPlatformMisc::GetPlatformUserForUserIndex(UserIndex);

      SaveSystem->DoesSaveGameExistAsync(*SlotName, PlatformUserId,
         [SaveExistsDelegate, UserIndex](const FString& SlotName, FPlatformUserId PlatformUserId, ISaveGameSystem::ESaveExistsResult Result)
         {
            check(IsInGameThread());
            SaveExistsDelegate.ExecuteIfBound(SlotName, UserIndex, Result);
         }
      );
   }
   else
   {
      SaveExistsDelegate.ExecuteIfBound(SlotName, UserIndex, ISaveGameSystem::ESaveExistsResult::UnspecifiedError);
   }
}

void UOSESaveGameSystem::LoadAsync()
{
   // Begin loading the player's save data asynchronously.
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("LoadAsync()"));

   // Has SaveGameSystem and Identity?
   if (!_IsReadyIO())
   {
      OnSaveIOFail.Broadcast(EOSESaveIOState::Loading);
      return;
   }

   // Create new save game?
   UGameplayStatics_DoesSaveGameExistAsync(*_saveSlotName, 0, _saveExistsAsyncCallbackDelegate);
}

void UOSESaveGameSystem::_SaveExistsCallback(const FString&, int32, ISaveGameSystem::ESaveExistsResult result)
{
   if (result != ISaveGameSystem::ESaveExistsResult::OK)
   {
      if (CreateNewSaveGame())
      {
         _LoadAsyncCallback(_saveSlotName, kDefaultSlotIndex, _saveData);
      }
      else
      {
         OnSaveIOFail.Broadcast(EOSESaveIOState::Creating);
      }
   }
   else
   {
      // Load game async. Calls `_LoadAsyncCallback` on result.
      UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("LoadAsync(): Loading-async slot %s [index=%i]..."), *_saveSlotName, 0);
      _SetIOState(EOSESaveIOState::Loading);
      UGameplayStatics::AsyncLoadGameFromSlot(_saveSlotName, kDefaultSlotIndex, _loadAsyncCallbackDelegate);
   }
}

void UOSESaveGameSystem::_LoadAsyncCallback(const FString&, const int32, USaveGame* loadedSaveData)
{
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("_LoadAsyncCallback()"));
   UOSESaveGame* newSaveData = Cast<UOSESaveGame>(loadedSaveData);
   newSaveData = _ResolveSaveDataClearVersion(newSaveData);
   if (newSaveData != nullptr)
   {
      newSaveData->SetSaveGameSystem(this);
      newSaveData->OnLoaded();
   }
   _SetSaveData(newSaveData);
   _SetIOState(EOSESaveIOState::NoOperation);

   // Callback on result from async 'load save game'.
   if (_saveDataState != EOSESaveDataState::HasSaveData)
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("_LoadAsyncCallback(): Load async failed (save game data is nullptr)"));
      OnSaveIOFail.Broadcast(EOSESaveIOState::Loading);
      return;
   }

   // Success
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("_LoadAsyncCallback(): Load async success"));
}

UOSESaveGame* UOSESaveGameSystem::_ResolveSaveDataClearVersion(UOSESaveGame* saveData)
{
   UOSESaveGame* newSaveData = saveData;
#if !UE_BUILD_SHIPPING
   if (newSaveData && newSaveData->GetClearVersion() < newSaveData->GetSaveCurrentClearVersion())
   {
      UE_LOG(LogOSESaveGameSystem, Log, TEXT("Save data loaded at clear version %d which is older than the current clear version %d.  Resetting save data."), newSaveData->GetClearVersion(), newSaveData->GetSaveCurrentClearVersion());
      newSaveData->MarkAsGarbage();
      newSaveData = Cast<UOSESaveGame>(UGameplayStatics::CreateSaveGameObject(_saveGameClass));
      if (newSaveData)
      {
         newSaveData->SetSaveGameSystem(this);
         newSaveData->OnCreated();
      }      
   }
#endif
   return newSaveData;
}

//--------------------------------------------------------------------------
// Save methods

bool UOSESaveGameSystem::Save()
{
   // Save player's current data to disk (blocking).
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("Save()"));

   // No save data?
   if (!HasLoadedSaveData())
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("Save(): Fail (save data is null)"));
      OnSaveIOFail.Broadcast(EOSESaveIOState::Saving);
      return false;
   }

   // Has SaveGameSystem and Identity?
   if (!_IsReadyIO())
   {
      // TODO: Do we need to attempt a resave when we fail?
      OnSaveIOFail.Broadcast(EOSESaveIOState::Saving);
      return false;
   }

   _ClearSaveTimer();

   // Save
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("Save(): Saving slot %s [index=%i]..."), *_saveSlotName, 0);
   _SetIOState(EOSESaveIOState::Saving);
   bool success = UGameplayStatics::SaveGameToSlot(_saveData, _saveSlotName, 0);
   if (success)
   {
      _saveData->OnSavedToDisk();
   }
   _SetSaveDataState(_saveData != nullptr ? EOSESaveDataState::HasSaveData : EOSESaveDataState::NoSaveData);
   _SetIOState(EOSESaveIOState::NoOperation);

   // Fail
   if (!success)
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("Save(): Failed to save"));
      OnSaveIOFail.Broadcast(EOSESaveIOState::Saving);
      return false;
   }

   // Success
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("Save(): Save success (write to disk)"));
   return true;
}

void UOSESaveGameSystem::SaveAsync()
{
   // Begin saving the player's current data asynchronously.
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("SaveAsync()"));

   // No save data?
   if (!HasLoadedSaveData())
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("Save(): Fail (save data is null)"));
      OnSaveIOFail.Broadcast(EOSESaveIOState::Saving);
      return;
   }

   // Set 'wants to save'. On write-to-disk async start,
   // clear flag. In case a save is triggered during IO,
   // this flag will start up another save.
   _pendingSave = true;

   if (_IsReadyIO())
   {
      // Clear 'wants to save'
      _pendingSave = false;
      _saveData->OnBeforeSaveToDisk();

      // Save game async. Calls `_SaveAsyncCallback` on result.
      UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("SaveAsync(): Saving-async slot %s [index=%i]..."), *_saveSlotName, 0);
      _SetIOState(EOSESaveIOState::Saving);
      UGameplayStatics::AsyncSaveGameToSlot(_saveData, _saveSlotName, kDefaultSlotIndex, _saveAsyncCallbackDelegate);
   }
}

void UOSESaveGameSystem::_SaveAsyncCallback(const FString& slotName, const int32 slotUserIndex, bool success)
{
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("_SaveAsyncCallback()"));
   if (success)
   {
      _saveData->OnSavedToDisk();
   }
   _SetSaveDataState(_saveData != nullptr ? EOSESaveDataState::HasSaveData : EOSESaveDataState::NoSaveData);
   _SetIOState(EOSESaveIOState::NoOperation);

   // Callback on result from async 'load save game'.
   if (!success)
   {
      // TODO: Do we need to attempt a resave when we fail?
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("_SaveAsyncCallback(): Save async fail"));
      OnSaveIOFail.Broadcast(EOSESaveIOState::Saving);
      return;
   }

   // Success
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("_SaveAsyncCallback(): Save async success (write to disk)"));

   // Pending save? (A new save timer fired during the
   // IO operation).
   if (_pendingSave)
   {
      UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("_SaveAsyncCallback(): Starting pending-save..."));
      SaveAsync();
   }
}



//--------------------------------------------------------------------------
// Create methods (used in `Load()`)

bool UOSESaveGameSystem::CreateNewSaveGame()
{
   // Create new save game data (writes to disk).
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("CreateNewSaveGame()"));

   // Create save game object (class from OSE Project Settings)
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("CreateNewSaveGame(): Creating save game object..."));
   _SetIOState(EOSESaveIOState::Creating);
   UOSESaveGame* newSaveData = Cast<UOSESaveGame>(UGameplayStatics::CreateSaveGameObject(_saveGameClass));
   if (newSaveData != nullptr)
   {
      newSaveData->SetSaveGameSystem(this);
      newSaveData->OnCreated();
   }
   _SetSaveData(newSaveData);
   _SetIOState(EOSESaveIOState::NoOperation);

   // Fail
   if (_saveDataState != EOSESaveDataState::HasSaveData)
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("CreateNewSaveGame(): Failed to instantiate new save game object"));
      OnSaveIOFail.Broadcast(EOSESaveIOState::Creating);
      return false;
   }

   // Success
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("CreateNewSaveGame(): New save game data created"));
   return true;
}



//--------------------------------------------------------------------------
// Delete methods

bool UOSESaveGameSystem::Delete()
{
   // Delete player's save data (object and on-disk, blocking).
   
   // Clear save data
   _SetSaveData(nullptr);
   
   // Save data exists?
   if (!UGameplayStatics::DoesSaveGameExist(_saveSlotName, 0))
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("Delete(): Cannot delete save game %s [%i] (does not exist)"), *_saveSlotName, 0);
      OnSaveIOFail.Broadcast(EOSESaveIOState::Deleting);
      return false;
   }

   _SetIOState(EOSESaveIOState::Deleting);
   bool success = UGameplayStatics::DeleteGameInSlot(_saveSlotName, 0);
   _SetIOState(EOSESaveIOState::NoOperation);

   // Fail
   if (!success)
   {
      OnSaveIOFail.Broadcast(EOSESaveIOState::Deleting);
      return false;
   }

   // Success
   return true;
}



//--------------------------------------------------------------------------
// Internal (private) save/load methods

UOSEPlatformIdentity* UOSESaveGameSystem::_GetPlatformIdentity() const
{
   UOSEIdentityMgr* identityMgr = _gameInstance->GetSubsystem<UOSEIdentityMgr>();
   check(identityMgr != nullptr);
   return identityMgr->GetPlatformIdentity();
}

void UOSESaveGameSystem::_OnLoginFlowLogout(const TArray<FString>& loginDomains)
{
   // Event handler on user 'logging out'.
   if (HasLoadedSaveData())
   {
      _SetSaveDataState(EOSESaveDataState::PreUnload);
   }
}

void UOSESaveGameSystem::_OnLoginStatusChanged(EOSEPlatformLoginState newState)
{
   // Event handler for identity 'login status change'.

   // `HasSaveData`, but logged out? Clear save data + set state
   if (HasLoadedSaveData() && newState == EOSEPlatformLoginState::NotLoggedIn)
   {
      UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("_OnLoginStatusChanged(): Logged-out (clearning save data object)"));
      _saveData = nullptr;
      _SetSaveDataState(EOSESaveDataState::NoSaveData);
   }
}

bool UOSESaveGameSystem::_IsReadyIO() const
{
   // Returns true if ready to save/load save game data (internal).

   // Already in IO state?
   if (_saveIOState != EOSESaveIOState::NoOperation)
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("Fail (already in an IO state)"));
      return false;
   }

#  if WITH_EDITOR
   // Editor? Always ready (return true)
   if (GEngine->IsEditor())
      return true;
#  endif

   // No 'save game system interface'? Return...
   if (_saveGameSystem == nullptr)
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("ISaveGameSystem is null (should be set in 'Initialize()')"));
      return false;
   }

   // No 'platform identity' ptr? Return ...
   UOSEPlatformIdentity* platformIdentity = _GetPlatformIdentity();
   if (platformIdentity == nullptr)
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("Platform Identity is null (should be set in 'Initialize()')"));
      return false;
   }

   // No 'user index'? Return ...
   int32 localPlayerIndex = platformIdentity->GetLocalPlayerIndex();
   if (localPlayerIndex == INDEX_NONE)
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("Local player index is INDEX_NONE (should be set in 'UOSEPlatformIdentity::_OnLocalPlayerAdded')"));
      return false;
   }

   // No 'unique net Id'? Return...
   FUniqueNetIdPtr uniqueNetId = platformIdentity->GetUniqueNetId();
   if (!uniqueNetId.IsValid())
   {
      UE_LOG(LogOSESaveGameSystem, Warning, TEXT("No UniqueNetId for player (set in 'OSEPlatformIdentity')"));
      return false;
   }

   // Valid
   return true;
}




//--------------------------------------------------------------------------
// Save timer system
void UOSESaveGameSystem::OnSaveDataMarkedDirty(EOSESavePriority priority)
{
   // Raised on save-data is marked dirty. Checks if the save is 'high-priority'
   // and conditionally clears/updates the timer to happen earlier.

   // Skip 'None' priority
   if (priority == EOSESavePriority::None)
   {
      return;
   }
   
   // Determine when new save would happen
   float intervalForPriority = _GetSaveTimerInterval(priority);

   if (intervalForPriority < _saveTimerInterval)
   {
      FDateTime newEndTime = FDateTime::Now() + FTimespan::FromSeconds(static_cast<double>(intervalForPriority));
      if (!_saveTimerHandle.IsValid() || newEndTime < _saveTimerEnd)
      {
         _saveTimerPriority = priority;
         _SetSaveTimerInterval(intervalForPriority);
      }
   }
}

/*static*/
float UOSESaveGameSystem::_GetSaveTimerInterval(EOSESavePriority priority)
{
   // Returns the interval of the save-timer for a priority.
   switch (priority)
   {
   case EOSESavePriority::HighPriority:
      // 1 frame
      return 0.0f; 

   default:
      // 30 seconds
      return 30.0f; 
   }
}

void UOSESaveGameSystem::_SetSaveTimerInterval(float saveInterval)
{
   // Set the save-timer interval (clears old timers).
   _ClearSaveTimer();
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("_SetSaveTimerInterval: starting timer with interval %f"), saveInterval);

   if (saveInterval > 0.0F)
   {
      _gameInstance->GetTimerManager().SetTimer(_saveTimerHandle, this, &UOSESaveGameSystem::_OnSaveTimerExpire, saveInterval, false);
   }
   else
   {
      _saveTimerHandle = _gameInstance->GetTimerManager().SetTimerForNextTick(this, &UOSESaveGameSystem::_OnSaveTimerExpire);
   }
   _saveTimerEnd = FDateTime::Now() + FTimespan::FromSeconds(saveInterval);
   _saveTimerInterval = saveInterval;
}

void UOSESaveGameSystem::_ClearSaveTimer()
{
   // Clear on-going timer (reset timer handle).
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("_ClearSaveTimer()"));

   // Clear current timer
   if (_saveTimerHandle.IsValid())
   {
      _gameInstance->GetTimerManager().ClearTimer(_saveTimerHandle);
   }

   // Reset members
   _saveTimerHandle = FTimerHandle();
   _saveTimerEnd = FDateTime();
   _saveTimerInterval = _GetSaveTimerInterval(EOSESavePriority::None);
   _saveTimerPriority = EOSESavePriority::None;
}

void UOSESaveGameSystem::_OnSaveTimerExpire()
{
   // Raised when a scheduled save timer expire (write-to-disk)
   UE_LOG(LogOSESaveGameSystem, Verbose, TEXT("_OnSaveTimerExpire: Save Timer Expire (priority=%s)"), *UEnum::GetDisplayValueAsText(_saveTimerPriority).ToString());

   // The timer should not be running if save-data is null
   check(_saveData);

   // If dirty, write-to-disk
   if (_saveData->IsDirty())
   {
      // Attempt save-async (restart timer for fail)
      SaveAsync();
   }

   // Clear save timer
   _ClearSaveTimer();
}

#if WITH_EDITOR
void UOSESaveGameSystem::_OnPostLoadMapWithWorld(UWorld* loadedWorld)
{
   const FString currentWorldURL = loadedWorld->URL.ToString();

   const UGameMapsSettings* gameMapsSettings = GetDefault<UGameMapsSettings>();
   const FString defaultMap = gameMapsSettings->GetGameDefaultMap();

   // If launching directly into (non-default) map, force a save data load as we're bypassing the normal bootload sequence
   if (!currentWorldURL.Contains(defaultMap))
   {
      UE_LOG(LogOSESaveGameSystem, Log, TEXT("Player loading into non-default map (%s); forcing a save data load operation..."), *loadedWorld->GetMapName());
      Load();
   }

   // Unbind after first map is loaded, to avoid unnecessary callbacks
   FCoreUObjectDelegates::PostLoadMapWithWorld.RemoveAll(this);
}
#endif // WITH_EDITOR

