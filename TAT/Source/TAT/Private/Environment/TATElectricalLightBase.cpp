// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Environment/TATElectricalLightBase.h"

// tat
#include "Interactables/Electrical/TATElectricalDeviceComponent.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATElectricalLightBase)

// Sets default values
ATATElectricalLightBase::ATATElectricalLightBase()
{
   PrimaryActorTick.bCanEverTick = false;

   _electricalDeviceComponent = CreateDefaultSubobject<UTATElectricalDeviceComponent>("ElectricalDevice");
}

bool ATATElectricalLightBase::CanBeInhibitedBy_Implementation(FGameplayTag inhibitorType) const
{
   return IsInhibitable;
}

FTATInhibitorPlacementInfo ATATElectricalLightBase::GetInhibitorPlacementInfo_Implementation() const
{
   // Child classes will almost certainly want to override this, but we can provide a simple baseline implementation here
   return FTATInhibitorPlacementInfo::Make(GetActorLocation(), GetActorRotation());
}
