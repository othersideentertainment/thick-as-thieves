// Copyright Epic Games, Inc. All Rights Reserved.
// Copyright 2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#include "OSEPerceptionDebugModule.h"

//OSE
#include "OSEGameplayDebuggerPerception.h"

//UE
#include "GameplayDebugger.h"

#define LOCTEXT_NAMESPACE "FOSEPerceptionDebugModule"

namespace OSEPerceptionDebugModule
{
   FName DebuggerCategoryName(TEXT("OSE Perception"));
}

void FOSEPerceptionDebugModule::StartupModule()
{
#if WITH_GAMEPLAY_DEBUGGER
   IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
   GameplayDebuggerModule.RegisterCategory(OSEPerceptionDebugModule::DebuggerCategoryName, IGameplayDebugger::FOnGetCategory::CreateStatic(&FOSEGameplayDebuggerPerception::MakeInstance), EGameplayDebuggerCategoryState::EnabledInGameAndSimulate, 8);
   GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

void FOSEPerceptionDebugModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
   IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
   GameplayDebuggerModule.UnregisterCategory(OSEPerceptionDebugModule::DebuggerCategoryName);
   GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FOSEPerceptionDebugModule, OSEPerceptionDebug)
