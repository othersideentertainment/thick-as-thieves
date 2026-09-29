// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "OnlineSubsystem.h"
#include "Delegates/DelegateSignatureImpl.inl"
#include "Interfaces/OnlineIdentityInterface.h"

#if defined(PLATFORM_XSX) && PLATFORM_XSX
// gdk
#include <GDKUserManager.h>
#endif

#include "OSEPlatformIdentity.generated.h"

class UGameInstance;
class ULocalPlayer;

/// OSE Login state (for state machine)
UENUM(BlueprintType)
enum class EOSEPlatformLoginState : uint8
{
   NotLoggedIn,               /// Not logging-in
   LoggingIn,                 /// Currently logging-in async
   LoggedIn,                  /// Logged-in
};

/// OSE Identity state (for state machine)
UENUM(BlueprintType)
enum class EOSEPlatformIdentityState : uint8
{
   NoIdentity,                /// Not logged in (cannot get save data)
   HasIdentity,               /// Logged in (has save data)
};

/// OSE Controller state (for state machine)
UENUM(BlueprintType)
enum class EOSEPlatformControllerState: uint8
{
   NoController,              /// Does not have controller
   HasController,             /// Has controller
};


DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSELoginAsyncStateChangedDelegate, EOSEPlatformLoginState, newState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSEIdentityStateChangedDelegate, EOSEPlatformIdentityState, newState);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSEControllerStateChangedDelegate, EOSEPlatformControllerState, newState);


//---------------------------------------------------------------------------------------
/// OSE Platform Identity 
///
/// A platform-generic identity system. Handles the acquisition of a player's identity, 
/// with an identity state system and events on state change.
/// 
/// On creation, binds to identity-relevant events (controller connected, login status
/// changed, etc.). Unbinds on deconstruct.
///
/// Used by OSEIdentityMgr.
//---------------------------------------------------------------------------------------
UCLASS()
class OSECORE_API UOSEPlatformIdentity : public UObject
{
   GENERATED_BODY()

public:

   // static 
   static const UOSEPlatformIdentity& Get(const UObject& contextObject);
   static UOSEPlatformIdentity& GetRef(const UObject& contextObject);

   /// On startup (OSE Identity Manger subsystem, with reference to GameInstance)
   void Initialize(UGameInstance* owningGameInstance);
   
   /// On shutdown (OSE Identity Manger subsystem)
   void Deinitialize();

   int32 GetLocalPlayerIndex() const { return _localPlayerIndex; }
   FUniqueNetIdPtr GetUniqueNetId() const { return _uniqueNetId; }  
   FPlatformUserId GetPlatformUserId() const { return _platformUserId; }

   //--------------------------------------------------------------------------
   // OSE Identity state machine
private:
   /// Current login state (intially 'NotLoggedIn'). 
   EOSEPlatformLoginState _loginState = EOSEPlatformLoginState::NotLoggedIn;

   /// Current OSE Identity state (initially 'NoIdentity'). 
   EOSEPlatformIdentityState _identityState = EOSEPlatformIdentityState::NoIdentity;

   /// Current Controller state (intially 'NoController'). 
   EOSEPlatformControllerState _controllerState = EOSEPlatformControllerState::NoController;

   /// Sets the 'logging in' state machine. Raises 'leave', then 
   /// 'enter' state (events). Events raised only on state changed.
   void _SetLoginState(EOSEPlatformLoginState newState);

   /// Sets the OSE Identity state machine. Raises 'leave', then 
   /// 'enter' state (events). Events raised only on state changed.
   void _SetIdentityState(EOSEPlatformIdentityState newState);

   /// Sets the 'has controller' state machine. Raises 'leave', then 
   /// 'enter' state (events). Events raised only on state changed.
   void _SetControllerState(EOSEPlatformControllerState newState);

   /// Update OSE Identity state based on an `ELoginStatus`.
   /// \see UOSEPlatformIdentity::_ApplicationHasReactivated
   /// \see UOSEPlatformIdentity::_OnLoginStatusChanged
   /// \see UOSEPlatformIdentity::_SetIdentityState
   void _UpdateIdentityFromLoginStatus(ELoginStatus::Type newLoginStatus);

public:
   /// Returns the current login-in state.
   /// \see UOSEPlatformIdentity::_SetLoggingInState
   UFUNCTION(BlueprintCallable, category = "Identity|OSE")
   EOSEPlatformLoginState GetLoginState() const { return _loginState; };
   
   /// Returns the current OSE Identity state. 
   /// \see UOSEPlatformIdentity::_SetIdentityState
   UFUNCTION(BlueprintCallable, category = "Identity|OSE")
   EOSEPlatformIdentityState GetOSEIdentityState() const { return _identityState; };

   /// Returns the current controller state. 
   /// \see UOSEPlatformIdentity::_SetControllerState
   UFUNCTION(BlueprintCallable, category = "Identity|OSE")
   EOSEPlatformControllerState GetControllerState() const { return _controllerState; };

   /// Logs the primary player in asynchronously. 
   /// Primary player is determined in `OnLocalPlayerAdded`.  
   /// Calls 'SetLoginState()', which raises identity events. 
   /// \return True if the async process started successfully.
   /// \see UOSEPlatformIdentity::_OnLocalPlayerAdded
   /// \see UOSEPlatformIdentity::_SetLoginState
   UFUNCTION(BlueprintCallable, category = "Identity|OSE")
   bool Login();

   //--------------------------------------------------------------------------
   // OSE Identity events
public:
   /// Returns true if the primary player is signed in. 
   UFUNCTION(BlueprintCallable, category="Identity|OSE")
   bool HasIdentity() const { return _identityState == EOSEPlatformIdentityState::HasIdentity; }

   /// Logging in on begin/end async event. 
   /// With enum `newState` param
   /// \see UOSEPlatformIdentity::Login
   /// \see UOSEPlatformIdentity::_SetLoginState
   UPROPERTY(BlueprintAssignable, category = "Identity|OSE")
   FOSELoginAsyncStateChangedDelegate OnLoginStateChange;

   /// OSE Identity on gained/lost event. 
   /// With enum `newState` param
   /// \see UOSEPlatformIdentity::_SetIdentityState
   UPROPERTY(BlueprintAssignable, category = "Identity|OSE")
   FOSEIdentityStateChangedDelegate OnIdentityStateChanged;

   /// OSE controller on connected/disconnected event. 
   /// With enum `newState` param
   /// \see UOSEPlatformIdentity::_SetControllerState
   UPROPERTY(BlueprintAssignable, category = "Identity|OSE")
   FOSEControllerStateChangedDelegate OnControllerStateChanged;

private:
   /// Reference to the current GameInstance
   /// \see UOSEPlatformMgr::Initialize
   UPROPERTY(Transient)
   UGameInstance* _gameInstance = nullptr;

   /// The primary player's local user index
   int32 _localPlayerIndex = INDEX_NONE;

   /// The primary player's unique net Id
   FUniqueNetIdPtr _uniqueNetId = FUniqueNetIdPtr();

   /// The primary player's platform user Id
   FPlatformUserId _platformUserId = FPlatformUserId();

   //--------------------------------------------------------------------------
   // Core/Identity event binds
private:
   /// On local player added/initialized (determine primary player/controller index). 
   /// From `UGameInstance`
   /// Called after 'Initialize()'
   void _OnLocalPlayerAdded(ULocalPlayer* localPlayer);
   FDelegateHandle _localPlayerAddedHandle = FDelegateHandle();

   /// On app/game reactivated (home screen, back to game etc.).
   /// Here, controller assignments, etc. may have all changed. 
   /// From `FCoreDelegates`.
   void _ApplicationHasReactivated();
   FDelegateHandle _applicationHasReactivatedHandle = FDelegateHandle();


   void _RefreshControllerConnections();

   // On controller connected/disconnected (turn on/off, per local userId).
   void _OnInputDeviceConnectionChanged(EInputDeviceConnectionState newConnectionState, FPlatformUserId platformUserId, FInputDeviceId inputDeviceId);
   FDelegateHandle _controllerConnectionChangedHandle = FDelegateHandle();

   /// On controller pairing changed (per platform user id).
   /// \see UOSEPlatformIdentity::OnIdentityControllerPairingChanged
   void _OnInputDevicePairingChanged(FInputDeviceId inputDeviceId, FPlatformUserId newUserPlatformId, FPlatformUserId oldUserPlatformId);
   FDelegateHandle _controllerPairingChangedHandle = FDelegateHandle();

   /// On controller pairing changed (per user identity/account).
   /// Bind from `IOnlineIdentity`.  
   /// \see UOSEPlatformIdentity::OnControllerPairingChanged  
   void _OnIdentityControllerPairingChanged(int localUserNum, FControllerPairingChangedUserInfo previousUser, FControllerPairingChangedUserInfo newUser);
   FDelegateHandle _identityControllerPairingChangedHandle = FDelegateHandle();

   /// On player login status changed (on signin/signout, etc.).
   /// Bind from `IOnlineIdentity`
   void _OnLoginStatusChanged(int32 localUserNum, ELoginStatus::Type oldStatus, ELoginStatus::Type newStatus, const FUniqueNetId& newId);
   FDelegateHandle _loginStatusChangedHandle[MAX_LOCAL_PLAYERS];


   //--------------------------------------------------------------------------
   // Platform-specific methods (implementations in subdirectories per-platform)
public:

   /// On 'no controller' detected.
   /// XSX: Show GDK 'controller required' dialog.
   void HandleNoControllerForPlatform();
};
