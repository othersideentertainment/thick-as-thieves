// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Online/TATCreateSessionCallbackProxy.h"

// tat
#include "Online/TATSessionParameters.h"

// ue
#include "CreateSessionCallbackProxy.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "OnlineSessionSettings.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCreateSessionCallbackProxy)


struct FTATOnlineSubsystemBPCallHelper
{
public:
   FTATOnlineSubsystemBPCallHelper(const TCHAR* callFunctionContext, UObject* worldContextObject, FName systemName = NAME_None);

   void QueryIDFromPlayerController(APlayerController* playerController);

   bool IsValid() const
   {
      return UserID.IsValid() && (OnlineSub != nullptr);
   }

public:
   FUniqueNetIdPtr UserID;
   IOnlineSubsystem* const OnlineSub;
   const TCHAR* FunctionContext;
};

FTATOnlineSubsystemBPCallHelper::FTATOnlineSubsystemBPCallHelper(const TCHAR* callFunctionContext, UObject* worldContextObject, FName systemName)
   : OnlineSub(Online::GetSubsystem(GEngine->GetWorldFromContextObject(worldContextObject, EGetWorldErrorMode::ReturnNull), systemName))
   , FunctionContext(callFunctionContext)
{
   if (OnlineSub == nullptr)
   {
      FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s - Invalid or uninitialized OnlineSubsystem"), FunctionContext), ELogVerbosity::Warning);
   }
}

void FTATOnlineSubsystemBPCallHelper::QueryIDFromPlayerController(APlayerController* playerController)
{
   UserID.Reset();

   APlayerState* playerState = nullptr;
   if (playerController != nullptr)
   {
      playerState = ToRawPtr(playerController->PlayerState);
   }

   if (playerState != nullptr)
   {
      UserID = playerState->GetUniqueId().GetUniqueNetId();
      if (!UserID.IsValid())
      {
         FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s - Cannot map local player to unique net ID"), FunctionContext), ELogVerbosity::Warning);
      }
   }
   else
   {
      FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s - Invalid player state"), FunctionContext), ELogVerbosity::Warning);
   }
}

//////////////////////////////////////////////////////////////////////////
// UCreateSessionCallbackProxy

UTATCreateSessionCallbackProxy::UTATCreateSessionCallbackProxy()
   : _createCompleteDelegate(FOnCreateSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnCreateCompleted))
   , _startCompleteDelegate(FOnStartSessionCompleteDelegate::CreateUObject(this, &ThisClass::OnStartCompleted))
   , _numPublicConnections(1)
{
}

UTATCreateSessionCallbackProxy* UTATCreateSessionCallbackProxy::CreateSession(UObject* worldContextObject, class APlayerController* playerController, bool publicSession, int32 publicConnections, bool useLAN, FGameplayTag map, ETATDifficulty difficulty)
{
   UTATCreateSessionCallbackProxy* proxy = NewObject<UTATCreateSessionCallbackProxy>();
   proxy->_playerControllerWeakPtr = playerController;
   proxy->_isPublic = publicSession;
   proxy->_numPublicConnections = publicConnections;
   proxy->_useLAN = useLAN;
   proxy->_mapTag = map;
   proxy->_difficulty = difficulty;
   proxy->_worldContextObject = worldContextObject;
   return proxy;
}

void UTATCreateSessionCallbackProxy::Activate()
{
   FTATOnlineSubsystemBPCallHelper helper(TEXT("CreateSession"), _worldContextObject.Get());
   helper.QueryIDFromPlayerController(_playerControllerWeakPtr.Get());

   if (helper.IsValid())
   {
      IOnlineSessionPtr sessions = helper.OnlineSub->GetSessionInterface();
      if (sessions.IsValid())
      {
         _createCompleteDelegateHandle = sessions->AddOnCreateSessionCompleteDelegate_Handle(_createCompleteDelegate);

         FOnlineSessionSettings settings;
         settings.NumPublicConnections = _numPublicConnections;
         settings.bShouldAdvertise = true;
         settings.bAllowJoinInProgress = true;
         settings.bIsLANMatch = _useLAN;
         settings.bUsesPresence = true;
         settings.bAllowJoinViaPresence = true;
         // This should cause the steam oss to select k_ELobbyTypeFriendsOnly if not public
         settings.bAllowJoinViaPresenceFriendsOnly = !_isPublic;
         settings.bUseLobbiesIfAvailable = true;

         namespace SP = TATSessionParameters;
         if (TOptional<int32> encodedMap = SP::EncodeMap(_mapTag))
         {
            settings.Set(SP::GetMapKey(), encodedMap.GetValue(), EOnlineDataAdvertisementType::ViaOnlineService);
         }
         settings.Set(SP::GetDifficultyKey(), SP::EncodeDifficulty(_difficulty), EOnlineDataAdvertisementType::ViaOnlineService);

         sessions->CreateSession(*helper.UserID, NAME_GameSession, settings);

         // OnCreateCompleted will get called, nothing more to do now
         return;
      }
      else
      {
         FFrame::KismetExecutionMessage(TEXT("Sessions not supported by Online Subsystem"), ELogVerbosity::Warning);
      }
   }

   // Fail immediately
   OnFailure.Broadcast();
}

void UTATCreateSessionCallbackProxy::OnCreateCompleted(FName sessionName, bool wasSuccessful)
{
   FTATOnlineSubsystemBPCallHelper helper(TEXT("CreateSessionCallback"), _worldContextObject.Get());
   helper.QueryIDFromPlayerController(_playerControllerWeakPtr.Get());

   if (helper.IsValid())
   {
      IOnlineSessionPtr sessions = helper.OnlineSub->GetSessionInterface();
      if (sessions.IsValid())
      {
         sessions->ClearOnCreateSessionCompleteDelegate_Handle(_createCompleteDelegateHandle);
         
         if (wasSuccessful)
         {
            _startCompleteDelegateHandle = sessions->AddOnStartSessionCompleteDelegate_Handle(_startCompleteDelegate);
            sessions->StartSession(NAME_GameSession);

            // OnStartCompleted will get called, nothing more to do now
            return;
         }
      }
   }

   if (!wasSuccessful)
   {
      OnFailure.Broadcast();
   }
}

void UTATCreateSessionCallbackProxy::OnStartCompleted(FName sessionName, bool wasSuccessful)
{
   FTATOnlineSubsystemBPCallHelper helper(TEXT("StartSessionCallback"), _worldContextObject.Get());
   helper.QueryIDFromPlayerController(_playerControllerWeakPtr.Get());

   if (helper.IsValid())
   {
      IOnlineSessionPtr sessions = helper.OnlineSub->GetSessionInterface();
      if (sessions.IsValid())
      {
         sessions->ClearOnStartSessionCompleteDelegate_Handle(_startCompleteDelegateHandle);
      }
   }

   if (wasSuccessful)
   {
      OnSuccess.Broadcast();
   }
   else
   {
      OnFailure.Broadcast();
   }
}

