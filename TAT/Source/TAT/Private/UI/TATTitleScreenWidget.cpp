// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATTitleScreenWidget.h"

// TAT
#include "Common/TATVersionEdition.h"

// OSE
#include "Identity/OSEPlatformIdentity.h"
#include "Identity/OSEUserPrivilege.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTitleScreenWidget)

DEFINE_LOG_CATEGORY_STATIC(LogTATTitleScreen, Log, All);

// #TODO_ves: Move login flow somewhere else like GI/Online/Social subsystem and remove this class

void UTATTitleScreenWidget::NativeOnActivated()
{
   _SetFlowState(ETATTitleScreenFlowState::None);

   Super::NativeOnActivated();
}

void UTATTitleScreenWidget::_SetFlowState(ETATTitleScreenFlowState newState)
{
   if (_flowState == newState)
   {
      return;
   }

   UE_LOG(LogTATTitleScreen, Verbose, TEXT("_SetFlowState: %s -> %s"), *UEnum::GetDisplayValueAsText(_flowState).ToString(), *UEnum::GetDisplayValueAsText(newState).ToString());

   _flowState = newState;
   OnFlowStateChange(newState);
}

void UTATTitleScreenWidget::StartTitleScreenLoginFlow()
{
   _LoginFlowStart();
}

void UTATTitleScreenWidget::_LoginFlowStart()
{
   UE_LOG(LogTATTitleScreen, Verbose, TEXT("_LoginFlowStart()"));
   _SetFlowState(ETATTitleScreenFlowState::LoggingIn);

   UOSEPlatformIdentity& platformIdentity = UOSEPlatformIdentity::GetRef(*this);

   if (platformIdentity.GetLoginState() == EOSEPlatformLoginState::LoggedIn)
   {
      UE_LOG(LogTATTitleScreen, Verbose, TEXT("StartLoginFlow(): Already logged-in (moving to save-data)"));
      _OnLoginStateChanged(EOSEPlatformLoginState::LoggedIn);
      return;
   }

   platformIdentity.OnLoginStateChange.AddDynamic(this, &UTATTitleScreenWidget::_OnLoginStateChanged);

   if (!platformIdentity.Login())
   {
      UE_LOG(LogTATTitleScreen, Warning, TEXT("StartLoginFlow(): Failed to start logging-in"));
      _SetFlowState(ETATTitleScreenFlowState::None);
      _LoginFlowEnd();
      OnFlowFail(ETATTitleScreenFlowState::LoggingIn);
   }
}

void UTATTitleScreenWidget::_LoginFlowEnd()
{
   UE_LOG(LogTATTitleScreen, Verbose, TEXT("_LoginFlowEnd()"));

   UOSEPlatformIdentity& platformIdentity = UOSEPlatformIdentity::GetRef(*this);

   platformIdentity.OnLoginStateChange.RemoveDynamic(this, &UTATTitleScreenWidget::_OnLoginStateChanged);
}

void UTATTitleScreenWidget::_OnLoginStateChanged(EOSEPlatformLoginState loginState)
{
   UE_LOG(LogTATTitleScreen, Verbose, TEXT("_OnLoginStateChanged(): %s"), *UEnum::GetDisplayValueAsText(loginState).ToString());
   _LoginFlowEnd();

   switch (loginState)
   {
      case EOSEPlatformLoginState::NotLoggedIn:
      {
         _SetFlowState(ETATTitleScreenFlowState::None);
         OnFlowFail(ETATTitleScreenFlowState::LoggingIn);
         break;
      }

      case EOSEPlatformLoginState::LoggedIn:
      {
         _PrivFlowStart();
         break;
      }

      default: break;
   }
}

void UTATTitleScreenWidget::_PrivFlowStart()
{
   UE_LOG(LogTATTitleScreen, Verbose, TEXT("_PrivFlowStart()"));
   UOSEUserPrivilege& userPriv = UOSEUserPrivilege::GetRef(*this);

   _SetFlowState(ETATTitleScreenFlowState::CheckingPrivilege);

   userPriv.OnPrivilegeStateChanged.AddDynamic(this, &UTATTitleScreenWidget::_OnPrivStateChanged);
   userPriv.CheckPrivileges();
}

void UTATTitleScreenWidget::_PrivFlowEnd()
{
   UE_LOG(LogTATTitleScreen, Verbose, TEXT("_PrivFlowEnd()"));
   UOSEUserPrivilege& userPriv = UOSEUserPrivilege::GetRef(*this);

   userPriv.OnPrivilegeStateChanged.RemoveDynamic(this, &UTATTitleScreenWidget::_OnPrivStateChanged);
}

void UTATTitleScreenWidget::_OnPrivStateChanged(EOSEUserPrivilegeState privState)
{
   UE_LOG(LogTATTitleScreen, Verbose, TEXT("_OnPrivStateChanged(): %s"), *UEnum::GetDisplayValueAsText(privState).ToString());

   if (privState == EOSEUserPrivilegeState::Checking)
   {
      return;
   }

   _PrivFlowEnd();

   UOSEUserPrivilege& userPriv = UOSEUserPrivilege::GetRef(*this);
   if (privState == EOSEUserPrivilegeState::Unknown || !userPriv.HasPrivilege(EUserPrivileges::Type::CanPlay))
   {
      _SetFlowState(ETATTitleScreenFlowState::None);
      OnFlowFail(ETATTitleScreenFlowState::CheckingPrivilege);
   }
   else
   {
      _SaveDataFlowStart();
   }
}

void UTATTitleScreenWidget::_SaveDataFlowStart()
{
   UE_LOG(LogTATTitleScreen, Verbose, TEXT("_SaveDataFlowStart()"));
   UOSESaveGameSystem* saveGameSystem = UOSESaveGameSystem::Get(this);
   check(saveGameSystem);

   // Get save data (and bind events)
   _SetFlowState(ETATTitleScreenFlowState::GettingSaveData);

   // Check 'HasSaveData'?
   // EDITOR: Auto-load on PIE begin
   if (saveGameSystem->HasLoadedSaveData())
   {
      UE_LOG(LogTATTitleScreen, Verbose, TEXT("_SaveDataFlowStart(): PIE start auto-loaded save data"));
      _OnSaveDataStateChanged(EOSESaveDataState::HasSaveData);
      return;
   }

   saveGameSystem->OnSaveDataStateChanged.AddDynamic(this, &UTATTitleScreenWidget::_OnSaveDataStateChanged);
   saveGameSystem->OnSaveIOFail.AddDynamic(this, &UTATTitleScreenWidget::_OnSaveIOFail);
   saveGameSystem->LoadAsync();

   // Success callback `_OnSaveDataStateChanged` (state == `HasSaveData`)
   // Fail callack `_OnSaveIOFail`
}

void UTATTitleScreenWidget::_SaveDataFlowEnd()
{
   UE_LOG(LogTATTitleScreen, Verbose, TEXT("_SaveDataFlowEnd()"));
   UOSESaveGameSystem* saveGameSystem = UOSESaveGameSystem::Get(this);
   check(saveGameSystem);

   // Unbind events
   saveGameSystem->OnSaveDataStateChanged.RemoveDynamic(this, &UTATTitleScreenWidget::_OnSaveDataStateChanged);
   saveGameSystem->OnSaveIOFail.RemoveDynamic(this, &UTATTitleScreenWidget::_OnSaveIOFail);
}

void UTATTitleScreenWidget::_OnSaveDataStateChanged(EOSESaveDataState saveDataState)
{
   UE_LOG(LogTATTitleScreen, Verbose, TEXT("_OnSaveDataStateChanged(): %s"), *UEnum::GetDisplayValueAsText(saveDataState).ToString());

   if (saveDataState == EOSESaveDataState::HasSaveData)
   {
      _SaveDataFlowEnd();
      _SetFlowState(ETATTitleScreenFlowState::Success);
   }

   // Fail conditions will be raised in `_OnSaveIOFail`
}

void UTATTitleScreenWidget::_OnSaveIOFail(EOSESaveIOState ioState)
{
   UE_LOG(LogTATTitleScreen, Verbose, TEXT("_OnSaveIOFail(): %s"), *UEnum::GetDisplayValueAsText(ioState).ToString());
   _SaveDataFlowEnd();

   _SetFlowState(ETATTitleScreenFlowState::None);
   OnFlowFail(ETATTitleScreenFlowState::GettingSaveData);
}
