// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATExclusiveSwitchSet.h"

// tat
#include "Interactables/TATExplicitTransitionToggle.h"

// ue5
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATExclusiveSwitchSet)

// Sets default values
ATATExclusiveSwitchSet::ATATExclusiveSwitchSet()
{
   bReplicates = true;
   NetDormancy = DORM_DormantAll;

   // need a location for replication to work
   USceneComponent* sceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
   RootComponent = sceneComp;
   sceneComp->Mobility = EComponentMobility::Static;
}

// Called when the game starts or when spawned
void ATATExclusiveSwitchSet::BeginPlay()
{
	Super::BeginPlay();
	
}
void ATATExclusiveSwitchSet::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATExclusiveSwitchSet, _inUseToggle);
}

void ATATExclusiveSwitchSet::AuthorityRegisterSwitchToggle(UTATExplicitTransitionToggleComponent* switchToggle)
{
   check(HasAuthority());
   check(switchToggle);

   if (_switchedToggles.AddUnique(switchToggle) >= 0)
   {
      switchToggle->OnStateOrTransitionChanged.AddUniqueDynamic(this, &ATATExclusiveSwitchSet::_OnToggleStateChanged);

      if (_IsToggleInUse(switchToggle))
      {
         _SetInUseToggle(switchToggle);
      }
   }
}

bool ATATExclusiveSwitchSet::_IsToggleInUse(const UTATExplicitTransitionToggleComponent* toggle) const
{
   return toggle != nullptr && (toggle->IsOn() != DefaultState || toggle->IsTransitioning());
}

const UTATExplicitTransitionToggleComponent* ATATExclusiveSwitchSet::_FindAnyToggleInUse() const
{
   const TObjectPtr<const UTATExplicitTransitionToggleComponent>* result = _switchedToggles.FindByPredicate([this](const UTATExplicitTransitionToggleComponent* toggle) { return _IsToggleInUse(toggle); });
   return result != nullptr ? *result : nullptr;
}

void ATATExclusiveSwitchSet::_SetInUseToggle(const UTATExplicitTransitionToggleComponent* toggle)
{
   if (_inUseToggle == toggle) return;

   FlushNetDormancy();
   _inUseToggle = toggle;
   _BroadcastInUseChanged();
}

void ATATExclusiveSwitchSet::_BroadcastInUseChanged()
{
   OnInUseChanged.Broadcast(_inUseToggle != nullptr);
}

void ATATExclusiveSwitchSet::_OnToggleStateChanged(bool isOn, bool isTransitioning)
{
   // just recalculate for now
   _SetInUseToggle(_FindAnyToggleInUse());
}

void ATATExclusiveSwitchSet::_OnRep_InUseToggle()
{
   _BroadcastInUseChanged();
}


