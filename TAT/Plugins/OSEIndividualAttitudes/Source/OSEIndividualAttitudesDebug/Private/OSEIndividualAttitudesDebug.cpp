// Copyright 2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEIndividualAttitudesDebug.h"

// ose
#include "OSEGameplayDebuggerIndividualAttitudes.h"

// ue
#include "GameplayDebugger.h"

#define LOCTEXT_NAMESPACE "FOSEIndividualAttitudesDebugModule"

namespace OSEIndividualAttitudesDebugModule
{
   FName DebuggerCategoryName(TEXT("OSE Individual Attitudes"));
}

void FOSEIndividualAttitudesDebugModule::StartupModule()
{
#if WITH_GAMEPLAY_DEBUGGER
   IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
   GameplayDebuggerModule.RegisterCategory(
      OSEIndividualAttitudesDebugModule::DebuggerCategoryName,
      IGameplayDebugger::FOnGetCategory::CreateStatic(&FOSEGameplayDebuggerIndividualAttitudes::MakeInstance),
      EGameplayDebuggerCategoryState::EnabledInGameAndSimulate,
      8
   );
   GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

void FOSEIndividualAttitudesDebugModule::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
   IGameplayDebugger& GameplayDebuggerModule = IGameplayDebugger::Get();
   GameplayDebuggerModule.UnregisterCategory(OSEIndividualAttitudesDebugModule::DebuggerCategoryName);
   GameplayDebuggerModule.NotifyCategoriesChanged();
#endif
}

#undef LOCTEXT_NAMESPACE
    
IMPLEMENT_MODULE(FOSEIndividualAttitudesDebugModule, OSEIndividualAttitudesDebug)
