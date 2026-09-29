// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Online/OSEGameState.h"

// ose
#include "Player/OSEPlayerState.h"
#include "VoiceOver/OSEVoiceOverControllerComponent.h"

// ue4
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEGameState)

DEFINE_LOG_CATEGORY_STATIC(LogOSEGameState, Log, All);

AOSEGameState::AOSEGameState(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   _voController = CreateDefaultSubobject<UOSEVoiceOverControllerComponent>(TEXT("VOController"));
}

/* static */
AOSEGameState* AOSEGameState::GetOSEGameState(const UObject* contextObj)
{
   check(contextObj);
   return Cast<AOSEGameState>(contextObj->GetWorld()->GetGameState());
}

void AOSEGameState::HandleBeginPlay()
{
   Super::HandleBeginPlay();
}

void AOSEGameState::AddPlayerState(APlayerState* playerState)
{
   Super::AddPlayerState(playerState);
   
   if (PlayerArray.Contains(playerState))
   {
      AOSEPlayerState* osePS = CastChecked<AOSEPlayerState>(playerState);

      // cache
      _osePlayerStates.AddUnique(osePS);
      
      // sort
      _SortOSEPlayerStates();

      // give our subclasses a chance to do some work before we broadcast
      _OnOSEPlayerStateAdded(osePS);

      // let other systems know
      OnPlayerStateAdded.Broadcast(osePS);

      if (osePS->IsLocalPlayerState())
      {
         OnLocalPlayerStateAdded.Broadcast(osePS);
      }
   }
}

void AOSEGameState::RemovePlayerState(APlayerState* playerState)
{
   Super::RemovePlayerState(playerState);

   if (!PlayerArray.Contains(playerState))
   {
      AOSEPlayerState* osePS = CastChecked<AOSEPlayerState>(playerState);
      
      // remove from cache
      _osePlayerStates.Remove(osePS);

      // ensure it's all in the correct order
      _SortOSEPlayerStates();

      // give our subclasses a chance to do some work before we broadcast
      _OnOSEPlayerStateRemoved(osePS);

      // let other systems know
      OnPlayerStateRemoved.Broadcast(osePS);
   }
}

void AOSEGameState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME(AOSEGameState, _sessionStats);
}

void AOSEGameState::AuthorityUpdateSessionStatInt(const FGameplayTag& statTag, int updateValue)
{
   check(HasAuthority());
   FOSEPlayerStat& stat = _sessionStats.GetOrAddStat(statTag);
   stat.IntValue += updateValue;
   _BroadcastSessionStatsChanged();
}

void AOSEGameState::_SortOSEPlayerStates()
{
   // this sort should work across clients + server to keep the list stable
   // even though PlayerArray is not due to being a replicated array
   _osePlayerStates.Sort([&](const AOSEPlayerState& lhs, const AOSEPlayerState& rhs)
   {
      return lhs.GetPlayerId() < rhs.GetPlayerId();
   });
}

void AOSEGameState::_OnRep_SessionStats()
{
   _BroadcastSessionStatsChanged();
}

void AOSEGameState::_BroadcastSessionStatsChanged()
{
   UE_LOG(LogOSEGameState, Verbose, TEXT("Session stats changed!"));
   OnSessionStatsChanged.Broadcast();
}

