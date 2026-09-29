// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "PingSystem/TATPingActor.h"

// TAT
#include "Net/TATIrisGroupSubsystem.h"
#include "Player/TATPlayerState.h"

// OSE
#include "OSECommon.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPingActor)

void ATATPingActor::BeginPlay()
{
   Super::BeginPlay();
   if(IsNetMode(NM_ListenServer))
   {
      const ATATPlayerState* playerState = ATATPlayerState::GetLocalTATPlayerState(this);
      if(_IsPingValidForPlayerState(playerState))
      {
         _TriggerPing();
      }
   }
   else
   {
      _TriggerPing();
   }
}

void ATATPingActor::BeginReplication()
{
   Super::BeginReplication();

   if(UTATIrisGroupSubsystem* groupSubsystem = GetWorld()->GetSubsystem<UTATIrisGroupSubsystem>())
   {
      if(ensure(GetPingedBy()))
      {
         groupSubsystem->ForTeam(GetPingedBy()->GetTeam()).AddActorToGroup(this);
      }
   }
}

bool ATATPingActor::IsNetRelevantFor(const AActor* realViewer, const AActor* viewTarget, const FVector& srcLocation) const
{
   check(realViewer && realViewer->HasAuthority());
   const ATATPlayerState* playerState = UOSECommon::GetPlayerState<const ATATPlayerState>(realViewer);
   return _IsPingValidForPlayerState(playerState);
}

bool ATATPingActor::_IsPingValidForPlayerState(const ATATPlayerState* playerState) const
{
   const AOSEPlayerState* const pingedBy = GetPingedBy();
   if(pingedBy == nullptr)
      return false;
   const EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(playerState, pingedBy);
   return attitude == EOSETeamAttitude::Friendly;
}

void ATATPingActor::_TriggerPing_Implementation()
{
}
