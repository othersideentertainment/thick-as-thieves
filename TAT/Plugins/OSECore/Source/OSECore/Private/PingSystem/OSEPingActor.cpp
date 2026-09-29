// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "PingSystem/OSEPingActor.h"

// ose
#include "PingSystem/OSEPingSystemComponent.h"
#include "Player/OSEPlayerController.h"

// ue4
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPingActor)

AOSEPingActor::AOSEPingActor()
   : Super()
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = true;
   bReplicates = true;
   bAlwaysRelevant = true;

   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
   PingUILocation = CreateDefaultSubobject<USceneComponent>(TEXT("PingUILocation"));
   PingUILocation->AttachToComponent(RootComponent, FAttachmentTransformRules::KeepRelativeTransform);
}

void AOSEPingActor::BeginPlay()
{
   Super::BeginPlay();

   // server bookkeeping for all actors spawned by this system
   if (_authorityPingSystemOwner)
   {
      _authorityPingSystemOwner->AuthorityAddSpawnedPingActor(this);
   }

   // local bookkeeping for ANY ping spawned by ANY player
   if (AOSEPlayerController* localPC = AOSEPlayerController::GetLocalOSEPlayerController(this))
   {
      if (UOSEPingSystemComponent* localPingSystem = localPC->GetPingSystem())
      {
         localPingSystem->LocallyAddSpawnedPingActor(this);
      }
   }
}

void AOSEPingActor::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);

   // server bookkeeping for all actors spawned by this system
   if (_authorityPingSystemOwner)
   {
      _authorityPingSystemOwner->AuthorityRemoveSpawnedPingActor(this);
      _authorityPingSystemOwner = nullptr;
   }

   // local bookkeeping for ANY ping spawned by ANY player
   if (AOSEPlayerController* localPC = AOSEPlayerController::GetLocalOSEPlayerController(this))
   {
      if (UOSEPingSystemComponent* localPingSystem = localPC->GetPingSystem())
      {
         localPingSystem->LocallyRemoveSpawnedPingActor(this);
      }
   }
}

void AOSEPingActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(AOSEPingActor, _pingTag);
   DOREPLIFETIME(AOSEPingActor, _pingedBy);
   DOREPLIFETIME(AOSEPingActor, _pingableActor);
}

void AOSEPingActor::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   // this ping should go away if it was associated w/ a pingable target actor and that target has gone away
   if (HasAuthority())
   {
      // TODO: Should the client be able to simulate this state and destroy locally ahead of the server?
      //       I am not sure that we really care about it being quite that responsive.

      bool isPingStillValid = true;
      if (_pingableActor.IsValid())
      {
         // figure out if it's still pingable, the interface allows turning this off whenever it wants to
         isPingStillValid = IOSEPingableInterface::Execute_IsPingable(_pingableActor.Get());
      }
      else if (_pingableActor.IsStale())
      {
         isPingStillValid = false;
      }

      if (!isPingStillValid)
      {
         check(_authorityPingSystemOwner);
         _authorityPingSystemOwner->ServerCancelPing(this, false);
      }
   }
}

void AOSEPingActor::AuthoritySetup(UOSEPingSystemComponent* pingSystemOwner, const FGameplayTag& pingTag, AActor* pingableTargetActor, const FHitResult& hitResult, const FTransform& spawnerViewXfm, const FTransform& spawnerWorldXfm)
{
   check(HasAuthority());
   check(pingSystemOwner);

   _pingTag = pingTag;
   _pingedBy = CastChecked<AOSEPlayerController>(pingSystemOwner->GetOwner())->GetOSEPlayerState();
   _pingableActor = pingableTargetActor;
   _authorityPingSystemOwner = pingSystemOwner;
   _hitResult = hitResult;
   _spawnerViewXfm = spawnerViewXfm;
   _spawnerWorldXfm = spawnerWorldXfm;
   if (PingDespawnTime > 0.0f)
      SetLifeSpan(PingDespawnTime);
   _OnAuthoritySetup();
}

bool AOSEPingActor::IsLocallyReadyToBeFocused_Implementation() const
{
   return true;
}

void AOSEPingActor::_OnRep_PingTag()
{
   _BroadcastPingTagChanged();
}

void AOSEPingActor::_BroadcastPingTagChanged()
{
   _OnPingTagChanged(_pingTag);
}

void AOSEPingActor::_OnPingTagChanged_Implementation(const FGameplayTag& pingTag)
{

}

void AOSEPingActor::_OnRep_PingedBy()
{
   _BroadcastPingedByChanged();
}

void AOSEPingActor::_BroadcastPingedByChanged()
{
   _OnPingedByChanged(_pingedBy);
}

void AOSEPingActor::_OnPingedByChanged_Implementation(AOSEPlayerState* player)
{

}

void AOSEPingActor::_OnRep_PingableActor()
{
   _BroadcastPingableActorChanged();
}

void AOSEPingActor::_BroadcastPingableActorChanged()
{
   _OnPingableActorChanged(_pingableActor.Get());
}

void AOSEPingActor::_OnPingableActorChanged_Implementation(AActor* pingableActor)
{
   // bp stub
}

void AOSEPingActor::_OnAuthoritySetup_Implementation()
{
   // bp stub
}

