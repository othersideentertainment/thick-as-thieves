// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// TAT
#include "UI/TATActivatableWidget.h"

// OSE
#include "Identity/OSEPlatformIdentity.h"
#include "Identity/OSESaveGameSystem.h"

#include "TATTitleScreenWidget.generated.h"

enum class EOSEUserPrivilegeState : uint8;

/// State machine for login flow task
UENUM(BlueprintType)
enum class ETATTitleScreenFlowState : uint8
{
   None,
   LoggingIn,
   CheckingPrivilege,
   GettingSaveData,
   Success,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATTitleScreenFlowStateChanged, ETATTitleScreenFlowState, state);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FTATTitleScreenFlowFail, ETATTitleScreenFlowState, state);

//---------------------------------------------------------------------------------------
/// TAT Title Screen Widget
/// 
/// A title screen widget base class, with login -> check privilege -> get save-data flow,
/// to validate a user's platform identity. Start flow with 'StartTitleScreenFlow', success
/// callback on 'OnFlowStateChanged(Success)' (explicit fail callback 'OnFlowFail'). 
/// Flow order: Identity -> Privilege -> Get Save-Data
/// 
/// Each flow state is asynchronous, and widget binds/unbinds during flow to relevant events.
/// 
/// Uses OSEIdentityMgr's sub-objects to run the flow and get the save data.
//---------------------------------------------------------------------------------------
UCLASS(MinimalAPI, Abstract, meta=(DisableNativeTick))
class UTATTitleScreenWidget : public UTATActivatableWidget
{
   GENERATED_BODY()

public:
   virtual void NativeOnActivated() override;

   /// On flow state change.
   /// \see UOSEPlatformIdentity::_SetIOState
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT", meta = (Tooltip = "Raised when title screen flow changes (login, saving)"))
   void OnFlowStateChange(ETATTitleScreenFlowState state);

   /// Flow failure.
   /// \see UTATTitleScreenWidget::_SetFlowState
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT", meta = (Tooltip = "Raised on login/save data fail"))
   void OnFlowFail(ETATTitleScreenFlowState state);

   UFUNCTION(BlueprintCallable, Category = "TAT")
   void StartTitleScreenLoginFlow();

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT", meta = (Tooltip = "Called when receiving an update regarding eta and position in the login queue"))
   void OnLoginQueueUpdate(int32 loginQueueEtaMilliseconds, int32 positionInQueue);

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT", meta = (Tooltip = "Called when the client is unable to connect to servers"))
   void OnLoginFailed();

private:
   /// Current login flow state for state machine
   UPROPERTY(BlueprintReadOnly, Category = "TAT", meta=(AllowPrivateAccess = "true"))
   ETATTitleScreenFlowState _flowState = ETATTitleScreenFlowState::None;

   /// Change login flow state (internal). 
   void _SetFlowState(ETATTitleScreenFlowState state);

   void _LoginFlowStart();
   void _LoginFlowEnd();

   /// Login-flow state changed callback. Binds in login-flow for
   /// success/fail, then unbinds in '_LoginFlowEnd()`.
   UFUNCTION()
   void _OnLoginStateChanged(EOSEPlatformLoginState loginState);

   void _PrivFlowStart();
   void _PrivFlowEnd();

   /// User privilege state changed callback. Bind to priv check for
   /// success/fail, then unbinds in '_PrivFlowEnd()'.
   UFUNCTION()
   void _OnPrivStateChanged(EOSEUserPrivilegeState privState);

   void _SaveDataFlowStart();
   void _SaveDataFlowEnd();

   /// Save data-flow fail callback (on save/load fail). Binds 
   /// in 'get save' flow, then unbinds in `_SaveDataFlowEnd()`.
   UFUNCTION()
   void _OnSaveIOFail(EOSESaveIOState ioState);

   /// Save data-flow success callback (if `HasSaveData`). Binds in
   /// 'get save' flow, then unbinds in `_SaveDataFlowEnd()`.
   UFUNCTION()
   void _OnSaveDataStateChanged(EOSESaveDataState saveDataState);

   FTimerHandle _loginFailedTimerHandle;
   UPROPERTY(EditDefaultsOnly, Category = "TAT")
   float _loginFailedMessageDuration = 3.0f;
};
