// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once
// Copyright 2023 OtherSide Entertainment, Inc. All Rights Reserved.

//UE
#include "GameplayDebuggerCategory.h"

#if WITH_GAMEPLAY_DEBUGGER
class FOSEGameplayDebuggerPerception :  public FGameplayDebuggerCategory
{
public:
   FOSEGameplayDebuggerPerception();

   //FGameplayDebuggerCategory Interface
   virtual void CollectData(APlayerController* OwnerPC, AActor* DebugActor) override;
   static TSharedRef<FGameplayDebuggerCategory> MakeInstance();
};

#endif //WITH_GAMEPLAY_DEBUGGER
