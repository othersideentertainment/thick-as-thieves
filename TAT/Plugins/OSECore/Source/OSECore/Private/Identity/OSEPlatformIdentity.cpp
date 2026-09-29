// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Identity/OSEPlatformIdentity.h"

// ose
#include "Identity/OSEIdentityMgr.h"

// ue4
#include "Engine.h"
#include "Delegates/DelegateSignatureImpl.inl"
#include "Interfaces/OnlineIdentityInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPlatformIdentity)

DEFINE_LOG_CATEGORY_STATIC(LogOSEPlatformIdentity, Log, All);

/* static */
const UOSEPlatformIdentity& UOSEPlatformIdentity::Get(const UObject& contextObject)
{
   return GetRef(contextObject);
}

/* static */
UOSEPlatformIdentity& UOSEPlatformIdentity::GetRef(const UObject& contextObject)
{
   UWorld* world = contextObject.GetWorld();
   check(world);
   UGameInstance* gameInstance = world->GetGameInstance();
   UOSEIdentityMgr* identityMgr = gameInstance->GetSubsystem<UOSEIdentityMgr>();
   check(identityMgr);
   UOSEPlatformIdentity* platformIdentity = identityMgr->GetPlatformIdentity();
   check(platformIdentity);
   return *platformIdentity;
}

void UOSEPlatformIdentity::Initialize(UGameInstance* owningGameInstance)
{
   // On startup (OSE Identity Manger subsystem)
   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("Initialize"));

   check(owningGameInstance);
   _gameInstance = owningGameInstance;


   //--------------------------------------
   // Core/Identity event binds

   _controllerConnectionChangedHandle = IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().AddUObject(this, &UOSEPlatformIdentity::_OnInputDeviceConnectionChanged);
   _controllerPairingChangedHandle = IPlatformInputDeviceMapper::Get().GetOnInputDevicePairingChange().AddUObject(this, &UOSEPlatformIdentity::_OnInputDevicePairingChanged);
   _applicationHasReactivatedHandle = FCoreDelegates::ApplicationHasReactivatedDelegate.AddUObject(this, &UOSEPlatformIdentity::_ApplicationHasReactivated);

   IOnlineSubsystem* subsystem = IOnlineSubsystem::Get();
   check(subsystem != nullptr);

   IOnlineIdentityPtr identity = subsystem->GetIdentityInterface();
   check(identity.IsValid());

   _identityControllerPairingChangedHandle = identity->AddOnControllerPairingChangedDelegate_Handle(FOnControllerPairingChangedDelegate::CreateUObject(this, &UOSEPlatformIdentity::_OnIdentityControllerPairingChanged));
   for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
   {
      _loginStatusChangedHandle[i] = identity->AddOnLoginStatusChangedDelegate_Handle(i, FOnLoginStatusChangedDelegate::CreateUObject(this, &UOSEPlatformIdentity::_OnLoginStatusChanged));
   }

   _localPlayerAddedHandle = _gameInstance->OnLocalPlayerAddedEvent.AddUObject(this, &UOSEPlatformIdentity::_OnLocalPlayerAdded);
}

void UOSEPlatformIdentity::Deinitialize()
{
   // On shutdown (OSE Identity Manger subsystem)
   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("Deinitialize"));

   //--------------------------------------
   // Core/Identity event unbinds

   IPlatformInputDeviceMapper::Get().GetOnInputDeviceConnectionChange().Remove(_controllerConnectionChangedHandle);
   IPlatformInputDeviceMapper::Get().GetOnInputDevicePairingChange().Remove(_controllerPairingChangedHandle);
   FCoreDelegates::ApplicationHasReactivatedDelegate.Remove(_applicationHasReactivatedHandle);

   IOnlineSubsystem* subsystem = IOnlineSubsystem::Get();
   check(subsystem != nullptr);

   IOnlineIdentityPtr identity = subsystem->GetIdentityInterface();
   check(identity.IsValid());

   identity->OnControllerPairingChangedDelegates.Remove(_identityControllerPairingChangedHandle);
   for (int i = 0; i < MAX_LOCAL_PLAYERS; i++)
   {
      identity->ClearOnLoginStatusChangedDelegate_Handle(i, _loginStatusChangedHandle[i]);
   }

   check(_gameInstance);
   _gameInstance->OnLocalPlayerAddedEvent.Remove(_localPlayerAddedHandle);
}

//--------------------------------------------------------------------------
// OSE Identity events

bool UOSEPlatformIdentity::Login()
{
   // Log-in the primary player (async).
  
   // Has primary player?
   if (_localPlayerIndex == INDEX_NONE)
   {
      UE_LOG(LogOSEPlatformIdentity, Warning, TEXT("Login(): Primary player unkown (localPlayerIndex==NONE)"));
      return false;
   }

   // Get OSS/identity
   IOnlineSubsystem* subsystem = IOnlineSubsystem::Get();
   IOnlineIdentityPtr identity = subsystem->GetIdentityInterface();
   check(identity.IsValid());
   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("Login(): Logging player in async ... (current loginStatus=%s)"), 
      ELoginStatus::ToString(identity->GetLoginStatus(_localPlayerIndex)));

   ELoginStatus::Type loginStatus = identity->GetLoginStatus(_localPlayerIndex);
   switch (loginStatus)
   {
   case ELoginStatus::NotLoggedIn:
      // Start the 'LoggingIn' async process. Attempt the platform's 
      // auto-login (raising `OnLoginStatusChanged`). On result, event 
      // `OnLoginStatusChanged` will enter the respective identity state.
      if (identity->AutoLogin(_localPlayerIndex))
      {
         _SetIdentityState(EOSEPlatformIdentityState::NoIdentity);
         _SetLoginState(EOSEPlatformLoginState::LoggingIn);
         return true;
      }
      break;

   case ELoginStatus::UsingLocalProfile:
   case ELoginStatus::LoggedIn:
      // Already logged in. Enter 'HasIdentity' state. 
      _SetIdentityState(EOSEPlatformIdentityState::HasIdentity);
      _SetLoginState(EOSEPlatformLoginState::LoggedIn);
      return true;
   }

   // Async login fail
   _SetLoginState(EOSEPlatformLoginState::NotLoggedIn);
   _SetIdentityState(EOSEPlatformIdentityState::NoIdentity);
   return false;
}

void UOSEPlatformIdentity::_SetLoginState(EOSEPlatformLoginState newState)
{
   // Already in 'new state'? Skip ...
   if (_loginState == newState)
      return;

   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("Set OSE Login State: entering '%s' (leaving '%s')"),
      *UEnum::GetDisplayValueAsText(newState).ToString(), *UEnum::GetDisplayValueAsText(_loginState).ToString());

   _loginState = newState;
   OnLoginStateChange.Broadcast(newState);
}

void UOSEPlatformIdentity::_SetIdentityState(EOSEPlatformIdentityState newState)
{
   // Already in 'new' state?
   if (_identityState == newState)
      return;

   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("Set OSE Identity: entering '%s' (leaving '%s')"),
      *UEnum::GetDisplayValueAsText(newState).ToString(), *UEnum::GetDisplayValueAsText(_identityState).ToString());

   _identityState = newState;
   OnIdentityStateChanged.Broadcast(newState);
}

void UOSEPlatformIdentity::_SetControllerState(EOSEPlatformControllerState newState)
{
   // Already in 'new state'? Skip ...
   if (_controllerState == newState)
      return;

   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("Set OSE Identity: entering '%s' (leaving '%s')"),
      *UEnum::GetDisplayValueAsText(newState).ToString(), *UEnum::GetDisplayValueAsText(_controllerState).ToString());

   _controllerState = newState;
   OnControllerStateChanged.Broadcast(newState);

   // Handle player has 'no controller' (platform-specific)
   if (newState == EOSEPlatformControllerState::NoController)
      HandleNoControllerForPlatform();
}

void UOSEPlatformIdentity::_UpdateIdentityFromLoginStatus(ELoginStatus::Type newLoginStatus)
{
   switch (newLoginStatus)
   {
   default:
   case ELoginStatus::NotLoggedIn:
      _SetLoginState(EOSEPlatformLoginState::NotLoggedIn);
      _SetIdentityState(EOSEPlatformIdentityState::NoIdentity);
      break;

   case ELoginStatus::UsingLocalProfile:
   case ELoginStatus::LoggedIn:
      _SetLoginState(EOSEPlatformLoginState::LoggedIn);
      _SetIdentityState(EOSEPlatformIdentityState::HasIdentity);
      break;
   }
}

//--------------------------------------------------------------------------
// Core/Identity events

void UOSEPlatformIdentity::_OnLocalPlayerAdded(ULocalPlayer* localPlayer)
{
   // On local player added/initialized (determine primary player/controller index). 
   // Called after ctor, as the local (primary) player is being added.
   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("OnLocalPlayerAdded(): name=\"%i\" controllerId=%i"),
      *localPlayer->GetNickname(), localPlayer->GetControllerId());

   // Intiailize as primary player?
   if (localPlayer->IsPrimaryPlayer())
   {
      _localPlayerIndex = _gameInstance->GetLocalPlayers().IndexOfByKey(localPlayer);

      IOnlineIdentityPtr identity = IOnlineSubsystem::Get()->GetIdentityInterface();
      check(identity.IsValid());

      _uniqueNetId = identity->GetUniquePlayerId(_localPlayerIndex);
      check(_uniqueNetId.IsValid());
      _platformUserId = identity->GetPlatformUserIdFromUniqueNetId(*_uniqueNetId);

      UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("OnLocalPlayerAdded(): Found primary player - index=%i =uniqueNetId=%s"),
         _localPlayerIndex, *_uniqueNetId->ToDebugString());
   }
}

void UOSEPlatformIdentity::_ApplicationHasReactivated()
{
   // On app/game reactivated (home screen, back to game etc.).
   // Here, controller assignments, etc. may have all changed. 
   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("ApplicationHasReactivated()"));

   // Update OSE Identity state from current login status
   IOnlineSubsystem* subsystem = IOnlineSubsystem::Get();
   IOnlineIdentityPtr identity = subsystem->GetIdentityInterface();
   check(identity.IsValid());

   // Update player info (controllerId, etc.)
   ULocalPlayer* player = _gameInstance->GetLocalPlayerByIndex(_localPlayerIndex);
   _RefreshControllerConnections();

   ELoginStatus::Type currentLoginStatus = identity->GetLoginStatus(_localPlayerIndex);
   _UpdateIdentityFromLoginStatus(currentLoginStatus);
   _uniqueNetId = identity->GetUniquePlayerId(_localPlayerIndex);
}

void UOSEPlatformIdentity::_RefreshControllerConnections()
{
   // just check if any controllers are connected, and update state
   // less fiddly than trying to mirror the state

   const IPlatformInputDeviceMapper& deviceMapper = IPlatformInputDeviceMapper::Get();
   TArray<FInputDeviceId> deviceIds;
   deviceMapper.GetAllInputDevicesForUser(_platformUserId, deviceIds);
   for (FInputDeviceId deviceId : deviceIds)
   {
      if(deviceMapper.GetInputDeviceConnectionState(deviceId) == EInputDeviceConnectionState::Connected)
      {
         _SetControllerState(EOSEPlatformControllerState::HasController);
         return;
      }
   }

   _SetControllerState(EOSEPlatformControllerState::NoController);
}

void UOSEPlatformIdentity::_OnInputDeviceConnectionChanged(EInputDeviceConnectionState newConnectionState, FPlatformUserId userPlatformId, FInputDeviceId inputDeviceId)
{
   // On controller connected/disconnected (turn on/off, per local user index).
   const bool connected = (newConnectionState == EInputDeviceConnectionState::Connected);
   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("OnControllerConnectionChanged(): connected=%s userPlatformId=%i inputDeviceId=%i"),
      connected ? TEXT("True") : TEXT("False"), userPlatformId.GetInternalId(), inputDeviceId.GetId());
   
   // Note: XSX calls this method on startup and controller power on/off, but
   // it does not raise this callback for controller pairing changes. Instead, 
   // XSX raises `_OnControllerPairingChanged`, so controller pairing swaps are 
   // handled there instead.

   // Is primary player?
   if (_localPlayerIndex != INDEX_NONE && _platformUserId == userPlatformId)
   {
      _RefreshControllerConnections();
   }
}

void UOSEPlatformIdentity::_OnInputDevicePairingChanged(FInputDeviceId inputDeviceId, FPlatformUserId newUserPlatformId, FPlatformUserId oldUserPlatformId)
{
   // On controller pairing changed (per user identity/account).
   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("OnControllerPairingChanged(): inputDeviceId=%i userId=%i (previous userId=%i)"),
      inputDeviceId.GetId(), newUserPlatformId.GetInternalId(), oldUserPlatformId.GetInternalId());


   // XSX: On pairing changed, if the primary player no longer has a controller, 
   // show "controller required" dialog. The dialog will be shown repeatedly until
   // the primary player has a controller (see `_XSXOnNoControllerDialogClosed`).
   if (_localPlayerIndex != INDEX_NONE)
   {
      ULocalPlayer* player = _gameInstance->GetLocalPlayerByIndex(_localPlayerIndex);

      // Connect?
      if (newUserPlatformId == _platformUserId || oldUserPlatformId == _platformUserId)
      {
         _RefreshControllerConnections();
      }
   }
}

void UOSEPlatformIdentity::_OnIdentityControllerPairingChanged(int localUserNum, FControllerPairingChangedUserInfo previousUser, FControllerPairingChangedUserInfo newUser)
{
   // On controller pairing changed (per platform user id).
   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("OnIdentityControllerPairingChanged(): localUserNum=%i user=%s (previous user=%s)"),
      localUserNum, *newUser.User.ToDebugString(), *previousUser.User.ToDebugString());
}

void UOSEPlatformIdentity::_OnLoginStatusChanged(int32 localUserNum, ELoginStatus::Type oldStatus, ELoginStatus::Type newStatus, const FUniqueNetId& newId)
{
   // On player login status changed (on signin/signout, etc.).
   // Potentially unneeded (required for XSX AdvancedUserModel)
   UE_LOG(LogOSEPlatformIdentity, Verbose, TEXT("OnLoginStatusChanged(): localUserNum=%i loginStatus=%s (previous loginStatus=%s), uniqueNetId=%s"),
      localUserNum, ELoginStatus::ToString(newStatus), ELoginStatus::ToString(oldStatus), *newId.ToDebugString());

   // Relevant to primary player? Re-check login status ...
   if (_localPlayerIndex != INDEX_NONE && _localPlayerIndex == localUserNum)
      _UpdateIdentityFromLoginStatus(newStatus);
}

