// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/TATGlyphTrap.h"

// tat
#include "Environment/TATOverlapTargetTriggerComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGlyphTrap)

// Sets default values
ATATGlyphTrap::ATATGlyphTrap()
{
   PrimaryActorTick.bCanEverTick = false;

   _overlapTargetTrigger = CreateDefaultSubobject<UTATOverlapTargetTriggerComponent>("OverlapTargetTrigger");
}

// Called when the game starts or when spawned
void ATATGlyphTrap::BeginPlay()
{
   Super::BeginPlay();

   if(HasAuthority())
   {
      if(IsArmed())
      {
         _overlapTargetTrigger->StartTracking();
      }
      _overlapTargetTrigger->OnTargetFound.AddUObject(this, &ATATGlyphTrap::_AuthorityOnTargetFound);
   }
}

void ATATGlyphTrap::_HandleStateChanged(const FTATTrapState& previousState)
{
   Super::_HandleStateChanged(previousState);

   if(HasAuthority())
   {
      if(IsArmed())
      {
         _overlapTargetTrigger->StartTracking();
      }
      else
      {
         _overlapTargetTrigger->StopTracking();
      }
   }
}

void ATATGlyphTrap::_AuthorityOnTargetFound(ETATOverlapTargetTriggerReason reason)
{
   if(IsArmed())
   {
      // arbitrarily pass first target
      check(_overlapTargetTrigger->GetCurrentTargets().Num() > 0);
      AuthorityTrigger(_overlapTargetTrigger->GetCurrentTargets()[0].Get());
   }
}
