// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Interfaces/OnlineSessionDelegates.h"
#include "Net/OnlineBlueprintCallProxyBase.h"
#include "GameplayTagContainer.h"

#include "TATCreateSessionCallbackProxy.generated.h"

class APlayerController;

enum class ETATDifficulty : uint8;

// Originally adapted from engine UCreateSessionCallbackProxy
UCLASS(MinimalAPI)
class UTATCreateSessionCallbackProxy : public UOnlineBlueprintCallProxyBase
{
   GENERATED_BODY()

public:
   UTATCreateSessionCallbackProxy();

   // Called when the session was created successfully
   UPROPERTY(BlueprintAssignable)
   FEmptyOnlineDelegate OnSuccess;

   // Called when there was an error creating the session
   UPROPERTY(BlueprintAssignable)
   FEmptyOnlineDelegate OnFailure;

   // Creates a session with the default online subsystem
   UFUNCTION(BlueprintCallable, DisplayName="Create Session [TAT]", meta=(BlueprintInternalUseOnly = "true", WorldContext="worldContextObject", Categories="Map"), Category = "Online|Session|TAT")
   static UTATCreateSessionCallbackProxy* CreateSession(UObject* worldContextObject, class APlayerController* playerController, bool publicSession, int32 publicConnections, bool useLAN, FGameplayTag map, ETATDifficulty difficulty);

   // UOnlineBlueprintCallProxyBase interface
   virtual void Activate() override;
   // End of UOnlineBlueprintCallProxyBase interface

private:
   // Internal callback when session creation completes, calls StartSession
   void OnCreateCompleted(FName sessionName, bool wasSuccessful);

   // Internal callback when session creation completes, calls StartSession
   void OnStartCompleted(FName sessionName, bool wasSuccessful);

   // The player controller triggering things
   TWeakObjectPtr<APlayerController> _playerControllerWeakPtr;

   // The delegate executed by the online subsystem
   FOnCreateSessionCompleteDelegate _createCompleteDelegate;

   // The delegate executed by the online subsystem
   FOnStartSessionCompleteDelegate _startCompleteDelegate;

   // Handles to the registered delegates above
   FDelegateHandle _createCompleteDelegateHandle;
   FDelegateHandle _startCompleteDelegateHandle;

   // Number of public connections
   int _numPublicConnections;

   // Whether or not to search LAN
   bool _useLAN;

   bool _isPublic = false;

   FGameplayTag _mapTag;
   ETATDifficulty _difficulty = static_cast<ETATDifficulty>(0);

   // The world context object in which this call is taking place
   TWeakObjectPtr<UObject> _worldContextObject;
};
