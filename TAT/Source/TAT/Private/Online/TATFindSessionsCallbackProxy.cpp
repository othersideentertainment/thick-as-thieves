// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Online/TATFindSessionsCallbackProxy.h"

// tat
#include "Online/TATSessionParameters.h"

// ue
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "OnlineSubsystem.h"
#include "OnlineSubsystemUtils.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "Online/OnlineSessionNames.h"
#include "FindSessionsCallbackProxy.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATFindSessionsCallbackProxy)

DEFINE_LOG_CATEGORY_STATIC(LogTATFindSessionsCallbackProxy, Log, All);

namespace FindSessionsHelpers
{
   struct FOnlineSubsystemBPCallHelper
   {
      FUniqueNetIdPtr UserID;
      IOnlineSubsystem* const OnlineSub;
      const TCHAR* FunctionContext;

      FOnlineSubsystemBPCallHelper(const TCHAR* callFunctionContext, UObject* worldContext, FName systemName = NAME_None)
         : OnlineSub(Online::GetSubsystem(GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::ReturnNull), systemName))
         , FunctionContext(callFunctionContext)
      {
         if (OnlineSub == nullptr)
         {
            FFrame::KismetExecutionMessage(*FString::Printf(TEXT("%s - Invalid or uninitialized OnlineSubsystem"), FunctionContext), ELogVerbosity::Warning);
         }
      }

      void QueryIDFromPlayerController(APlayerController* playerController)
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

      bool IsValid() const
      {
         return UserID.IsValid() && (OnlineSub != nullptr);
      }
   };
}

UTATFindSessionsCallbackProxy::UTATFindSessionsCallbackProxy()
   : _delegate(FOnFindSessionsCompleteDelegate::CreateUObject(this, &ThisClass::OnCompleted))
{
}

UTATFindSessionsCallbackProxy* UTATFindSessionsCallbackProxy::FindSessions(UObject* worldContext, APlayerController* playerController, const FTATSessionSearchQuery& sessionQuery)
{
   UTATFindSessionsCallbackProxy* proxy = NewObject<UTATFindSessionsCallbackProxy>();
   proxy->_playerControllerWeak = playerController;
   proxy->_sessionQuery = sessionQuery;
   proxy->_worldContext = worldContext;
   return proxy;
}

void UTATFindSessionsCallbackProxy::Activate()
{
   FindSessionsHelpers::FOnlineSubsystemBPCallHelper helper(TEXT("TATFindSessions"), _worldContext);
   helper.QueryIDFromPlayerController(_playerControllerWeak.Get());

   bool forceDedicatedSessionSearch = false;
   {
      int32 value = 0;
      FParse::Value(FCommandLine::Get(), TEXT("-ForceDedicatedSessionSearch="), value);
      forceDedicatedSessionSearch = value != 0;
   }

   if (helper.IsValid())
   {
      IOnlineSessionPtr sessions = helper.OnlineSub->GetSessionInterface();
      if (sessions.IsValid())
      {
         _delegateHandle = sessions->AddOnFindSessionsCompleteDelegate_Handle(_delegate);

         _searchObject = MakeShareable(new FOnlineSessionSearch);
         _searchObject->TimeoutInSeconds = FMath::Max(0.0f, _sessionQuery.TimeoutInSeconds);
         _searchObject->MaxSearchResults = _sessionQuery.MaxResults;
         _searchObject->bIsLanQuery = _sessionQuery.SearchLANOnly;

         if (forceDedicatedSessionSearch)
         {
            _searchObject->QuerySettings.Set(SEARCH_DEDICATED_ONLY, true, EOnlineComparisonOp::Equals);
            _searchObject->QuerySettings.Set(SEARCH_LOBBIES, false, EOnlineComparisonOp::Equals);
         }
         else
         {
            _searchObject->QuerySettings.Set(SEARCH_DEDICATED_ONLY, _sessionQuery.SearchDedicatedOnly, EOnlineComparisonOp::Equals);
         }

         _searchObject->QuerySettings.Set(SEARCH_EMPTY_SERVERS_ONLY, _sessionQuery.SearchEmptyServersOnly, EOnlineComparisonOp::Equals);
         _searchObject->QuerySettings.Set(SEARCH_NONEMPTY_SERVERS_ONLY, _sessionQuery.SearchNonEmptyServersOnly, EOnlineComparisonOp::Equals);

         if (_sessionQuery.SearchMinSlotsAvailable >= 1)
         {
            _searchObject->QuerySettings.Set(SEARCH_MINSLOTSAVAILABLE, _sessionQuery.SearchMinSlotsAvailable, EOnlineComparisonOp::GreaterThanEquals);
         }
         if (!_sessionQuery.SearchKeywords.IsEmpty())
         {
            _searchObject->QuerySettings.Set(SEARCH_KEYWORDS, _sessionQuery.SearchKeywords, EOnlineComparisonOp::In);
         }

         _searchObject->QuerySettings.Set(SEARCH_LOBBIES, _sessionQuery.SearchLobbies && !_sessionQuery.SearchDedicatedOnly, EOnlineComparisonOp::Equals);

         sessions->FindSessions(*helper.UserID, _searchObject.ToSharedRef());

         // OnQueryCompleted will get called, nothing more to do now
         return;
      }
      else
      {
         FFrame::KismetExecutionMessage(TEXT("Sessions not supported by Online Subsystem"), ELogVerbosity::Warning);
      }
   }

   // Fail immediately
   TArray<FBlueprintSessionResult> results;
   OnFailure.Broadcast(results);
}

void UTATFindSessionsCallbackProxy::OnCompleted(bool success)
{
   FindSessionsHelpers::FOnlineSubsystemBPCallHelper helper(TEXT("TATFindSessionsCallback"), _worldContext);
   helper.QueryIDFromPlayerController(_playerControllerWeak.Get());

   if (helper.IsValid())
   {
      IOnlineSessionPtr sessions = helper.OnlineSub->GetSessionInterface();
      if (sessions.IsValid())
      {
         sessions->ClearOnFindSessionsCompleteDelegate_Handle(_delegateHandle);
      }
   }

   TArray<FBlueprintSessionResult> results;

   if (success && _searchObject.IsValid())
   {
      for (FOnlineSessionSearchResult& searchResult : _searchObject->SearchResults)
      {
         FBlueprintSessionResult blueprintResult{};
         blueprintResult.OnlineResult = searchResult;
         results.Add(blueprintResult);
      }

      OnSuccess.Broadcast(results);
   }
   else
   {
      OnFailure.Broadcast(results);
   }
}

// static
bool UTATFindSessionsCallbackProxy::GetSessionSettingStringValue(const FBlueprintSessionResult& result, FName settingName, FString& value)
{
   return result.OnlineResult.Session.SessionSettings.Get(settingName, value);
}

// static
bool UTATFindSessionsCallbackProxy::GetSessionSettingIntValue(const FBlueprintSessionResult& result, FName settingName, int32& value)
{
   return result.OnlineResult.Session.SessionSettings.Get(settingName, value);
}

bool UTATFindSessionsCallbackProxy::GetSessionSettingMap(const FBlueprintSessionResult& result, FGameplayTag& mapTag)
{
   namespace SP = TATSessionParameters;
   int32 encodedMap;
   if (result.OnlineResult.Session.SessionSettings.Get(SP::GetMapKey(), encodedMap))
   {
      mapTag = SP::DecodeMap(encodedMap);
      return mapTag.IsValid();
   }

   return false;
}

bool UTATFindSessionsCallbackProxy::GetSessionSettingDifficulty(const FBlueprintSessionResult& result, ETATDifficulty& difficulty)
{
   namespace SP = TATSessionParameters;
   int32 encoded;
   if (result.OnlineResult.Session.SessionSettings.Get(SP::GetDifficultyKey(), encoded))
   {
      difficulty = SP::DecodeDifficulty(encoded);
      return true;
   }

   return false;
}

