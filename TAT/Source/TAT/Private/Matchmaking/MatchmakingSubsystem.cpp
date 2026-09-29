// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Matchmaking/MatchmakingSubsystem.h"

// tat
#include "TATGameInstance.h"
#include "GameFramework/TATTravelMgr.h"

// ose
#include "Identity/OSEPlatformIdentity.h"

// ue4
#include "Async/Async.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(MatchmakingSubsystem)

DEFINE_LOG_CATEGORY(LogTATMatchmaking);

namespace TATMatchmakingParams
{
   const float kWaitingForMatchmakingSeconds = 3.0f;
}

UTATMatchmakingSubsystem::UTATMatchmakingSubsystem()
{

}

void UTATMatchmakingSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
}

void UTATMatchmakingSubsystem::Deinitialize()
{
}

bool UTATMatchmakingSubsystem::StartMatchmaking()
{
   if (_matchmakingState == EMatchmakingState::None)
   {
      //return _StartMatchmaking();
      return false;
   }
   return false;
}

bool UTATMatchmakingSubsystem::RespondToMatchmakingTicket(bool accept)
{
   if (_matchmakingState == EMatchmakingState::WaitingForMatchmakingTicketResponse)
   {
      // accept ticket
      //return _AcceptMatchmakingTicket(accept);
      return false;
   }
   return false;
}

bool UTATMatchmakingSubsystem::StopMatchmaking()
{
   if (_matchmakingState != EMatchmakingState::None)
   {
      // if we don't have a ticket generated yet there's nothing to cancel
      if (!_matchmakingConnectionInfo.MatchmakingTicketId.IsEmpty())
      {
         // stop matchmaking for the current ticket
         // _StopMatchmaking();

         // reset matchmaking for re-initialization
         _ResetMatchmaking();
      }
      return true;
   }
   return false;
}

void UTATMatchmakingSubsystem::Tick(float deltaTime)
{
   switch(_matchmakingState)
   {
   case EMatchmakingState::SearchingForMatch:
   case EMatchmakingState::PlacingMatchOnServer:
      {
         if (!_isDescribingMatchmaking)
         {
            _describeMatchmakingSecondsRemaining -= deltaTime;
            if (_describeMatchmakingSecondsRemaining <= 0.0f)
            {
               //_DescribeMatchmaking()
            }
         }
      }
      break;
   }
}

void UTATMatchmakingSubsystem::_SetState(EMatchmakingState state)
{
   if (state != _matchmakingState)
   {
      EMatchmakingState prevState = _matchmakingState;
      UE_LOG(LogTATMatchmaking, Verbose, TEXT("Going from matchmaking state %s to %s"), *UEnum::GetValueAsString(prevState), *UEnum::GetValueAsString(state));
      _matchmakingState = state;
      OnMatchmakingStateChanged.Broadcast(state);
   }
}

void UTATMatchmakingSubsystem::_SetErrorState(EMatchmakingErrorState errorState)
{
   if (errorState != _matchmakingErrorState)
   {
      EMatchmakingErrorState prevState = _matchmakingErrorState;
      UE_LOG(LogTATMatchmaking, Verbose, TEXT("Going from error state %s to %s"), *UEnum::GetValueAsString(prevState), *UEnum::GetValueAsString(errorState));
      _matchmakingErrorState = errorState;
      // allow them to try again...
      _SetState(EMatchmakingState::None);
      OnMatchmakingErrorStateChanged.Broadcast(_matchmakingErrorState);
   }
}

void UTATMatchmakingSubsystem::_JoinSessionFromMatchmakingTicket()
{
   UE_LOG(LogTATMatchmaking, Log, TEXT("Joining server @ IP %s, Port %d"), *_matchmakingConnectionInfo.IPAddress, _matchmakingConnectionInfo.Port);

   FURL url;
   url.Host = _matchmakingConnectionInfo.IPAddress;
   url.Port = _matchmakingConnectionInfo.Port;

   // TODO: These magic strings live in UOSEDedicatedServerGameliftSettings which isn't currently compiled into the client.
   //       I should probably move it somewhere more shared...
   TArray<FString> options;
   options.Add(*FString::Printf(TEXT("playerSessionId=%s"), *_matchmakingConnectionInfo.PlayerSessionId));
   options.Add(*FString::Printf(TEXT("gameSessionId=%s"), *_matchmakingConnectionInfo.GameSessionId));

   // TODO: Let's not deal with timeouts right now while we stand up our matchmaking tests.
   // Espectially in uncooked editor builds running -game we will get killed on timeouts.
   // But also, like, our level can take a few minutes to load up as a client, and I am not ready to pump up the ConnectionTimeout ini value over it
   options.Add(*FString::Printf(TEXT("NoTimeouts")));
   
   for (const FString& opt : options)
   {
      url.AddOption(*opt);
      UE_LOG(LogTATMatchmaking, Log, TEXT("   - %s"), *opt);
   }

   UTATGameInstance& gameInstance = UTATGameInstance::Get(GetWorld());
   UTATTravelMgr& travelMgr = gameInstance.GetTravelMgr();
   travelMgr.ShowLoadingScreen();

   FWorldContext* context = GetGameInstance()->GetWorldContext();
   check(context);
   FString error;
   EBrowseReturnVal::Type value = GEngine->Browse(*context, url, error);
}

void UTATMatchmakingSubsystem::_ResetMatchmaking()
{
   // reset the current ticket / connection info
   _matchmakingConnectionInfo = FMatchmakingConnectionInfo();

   // back to no-state
   _SetState(EMatchmakingState::None);
}

FString UTATMatchmakingSubsystem::_GetUniqueNetIdStr() const
{
   const UOSEPlatformIdentity& platformIdentity = UOSEPlatformIdentity::Get(*this);
   FUniqueNetIdPtr uniqueNetId = platformIdentity.GetUniqueNetId();
   check(uniqueNetId.IsValid());
   return uniqueNetId->ToString();
}

