// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSEEngineSubsystem.h"
#include "AbilitySystemGlobals.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEEngineSubsystem)

void UOSEEngineSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
   Super::Initialize(Collection);

   UAbilitySystemGlobals::Get().InitGlobalData();
}

