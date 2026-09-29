// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

//UE
#include "GameplayDebuggerCategory.h"

#if WITH_GAMEPLAY_DEBUGGER
class FOSEGameplayDebuggerIndividualAttitudes : public FGameplayDebuggerCategory
{
public:
   FOSEGameplayDebuggerIndividualAttitudes();

   //FGameplayDebuggerCategory Interface
   virtual void CollectData(APlayerController* ownerPC, AActor* debugActor) override;
   static TSharedRef<FGameplayDebuggerCategory> MakeInstance();
};
#endif
