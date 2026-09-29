// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "Online.h"
#include "OnlineSubsystem.h"
#include "Delegates/DelegateSignatureImpl.inl"
#include "Interfaces/OnlineIdentityInterface.h"

#include "OSEUserPrivilege.generated.h"

class UGameInstance;

/// OSE User Privilege state (for state machine)
UENUM(BlueprintType)
enum class EOSEUserPrivilegeState : uint8
{
   Unknown,                   ///< Privileges unknown, not checked yet (initial state)
   Checking,                  ///< Currently checking privileges async
   Known,                     ///< User privileges are known
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOSEPrivilegeStateChangedDelegate, EOSEUserPrivilegeState, newState);

//---------------------------------------------------------------------------------------
/// OSE User Privilege
///
/// Manages acquiring a user's platform privileges asynchronously, with a ‘has checked 
/// privileges’ state machine (unknown, currently checking, known). User privilege once
/// found is cached into a priavate array, but with a public 'HasPrivilege(type)'.
/// 
/// Certain events - about to connect to voice-chat, application un-minimized - will
/// re-query the users privileges, checking for updates.
/// 
/// Platform-subclasses organize the handling of the platform unique behavior. A static
/// 'Create' function, declared in the base class, defined in the platform source, will
/// return the platform's derived class.
///
/// Used by OSEIdentityMgr.
//---------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API UOSEUserPrivilege : public UObject
{
   GENERATED_BODY()

public:

   // static 
   static const UOSEUserPrivilege& Get(const UObject& contextObject);
   static UOSEUserPrivilege& GetRef(const UObject& contextObject);

   /// Create the OSE User Privilege object.
   /// Stub, defined in platform 'cpp'.
   static UOSEUserPrivilege* Create(UObject* outer);

   /// On startup (OSE Identity Manger subsystem, with reference to GameInstance)
   void Initialize(UGameInstance* owningGameInstance);

   /// On shutdown (OSE Identity Manger subsystem)
   virtual void Deinitialize();

   /// Derived platform initialize callbacks
   virtual void InitializePlatform() {};
   virtual void DeinitializePlatform() {};

protected:
   /// Reference to the current GameInstance.
   /// \see UOSEIdentityMgr::Initialize
   UPROPERTY(Transient)
   UGameInstance* _gameInstance = nullptr;

   /// Bit flags per 'EUserPrivileges::Type' (cached privileges).
   uint32 _userKnownHasPrivFlags;

   // Array of relevant privilige enum types
   TArray<EUserPrivileges::Type> _privilegeTypes;


   //--------------------------------------------------------------------------
   // State machine (has checked privilege)
protected:
   /// Current privilege state (default 'Unknown').
   EOSEUserPrivilegeState _knowsPrivilegesState = EOSEUserPrivilegeState::Unknown;

   /// Sets the 'privilege' state machine. Raises 'state change'
   /// event. If already in-state, returns.
   void _SetPrivilegeState(EOSEUserPrivilegeState newState);

public:
   /// Returns true if the users privilige is known (was checked).
   UFUNCTION(BlueprintCallable)
   bool HasCheckedPrivileges() const { return _knowsPrivilegesState == EOSEUserPrivilegeState::Known; }

   /// Returns the current 'privilege' state.
   UFUNCTION(BlueprintCallable)
   EOSEUserPrivilegeState GetUserPrivilegeState() const { return _knowsPrivilegesState; }

   /// Privilege check begin/end async event. 
   /// With enum `newState` param
   /// \see UOSEUserPrivilege::_SetPrivilegeState
   FOSEPrivilegeStateChangedDelegate OnPrivilegeStateChanged;


   //--------------------------------------------------------------------------
   // Public methods
public:
   /// Returns true if the user's has a privilege, or false  if 
   /// privileges are unknown, or the user's account is restricted
   /// for the privilege type.
   bool HasPrivilege(EUserPrivileges::Type privilege) const;

   /// Begin checking a user's privileges (async). Checks each 
   /// category of user privileges, result callback per-type in 
   /// `_OnPrivilegeCheckCompleteCallback`.
   bool CheckPrivileges();

   /// Re-check privileges. Currently, just calls `CheckPrivileges`.
   bool RefreshPrivileges();


   //--------------------------------------------------------------------------
   // Async check callbacks
private:
   /// Identity privilege check callback. Returns result of async-
   /// checking a user's privileges (per-priv type).
   void _OnPrivilegeCheckCompleteCallback(const FUniqueNetId& localUserId, EUserPrivileges::Type privilege, uint32 privilegeResult);
   IOnlineIdentity::FOnGetUserPrivilegeCompleteDelegate _OnGetUserPrivilegeCompleteDelegate;

   /// Number of 'check privilege category' callbacks recieved
   int _outstandingAsyncTasks = 0;

};
