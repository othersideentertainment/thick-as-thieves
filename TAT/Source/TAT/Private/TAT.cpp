// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "TAT.h"

// tat
#include "Breakables/TATBreakableDebugVis.h"

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebugger.h"
#include "AI/Utility/GameplayDebuggerCategory_UtilityStates.h"
#include "AI/Utility/GameplayDebuggerCategory_TATUtilityTargets.h"
#include "Developer/GameplayDebuggerCategory_TATStealth.h"
#endif

// ue5
#include "GameFramework/HUD.h"
#include "Modules/ModuleManager.h"

void FTAT::StartupModule()
{

#if !(UE_BUILD_SHIPPING || UE_BUILD_TEST)
   AHUD::OnShowDebugInfo.AddStatic(&BreakableDebugVis::OnShowDebugInfo);
#endif

#if WITH_GAMEPLAY_DEBUGGER
   IGameplayDebugger& gameplayDebuggerModule = IGameplayDebugger::Get();
   gameplayDebuggerModule.RegisterCategory("Stealth", IGameplayDebugger::FOnGetCategory::CreateStatic(&FGameplayDebuggerCategory_TATStealth::MakeInstance));
   gameplayDebuggerModule.NotifyCategoriesChanged();
#endif

   // Enable mobile text scaling while on steam deck
   //
   // Using SteamDeck=1 environment variable as heuristic, which is not officially
   // documented, but is relied upon enough by other games that Valve is unlikely to
   // change it. There is an official steam api, but that may not be initialized
   // by the time that this module starts.
   //
   // Also -PretendSteamDeck on the command line for testing
   if((FPlatformMisc::GetEnvironmentVariable(TEXT("SteamDeck")) == TEXT("1")) ||
      FParse::Param(FCommandLine::Get(), TEXT("PretendSteamDeck")))
   {
      // This Cvar is in the commonui plugin, which this module depends on, so it
      // should exist at this point.
      if (IConsoleVariable* textScalingCvar = IConsoleManager::Get().FindConsoleVariable(TEXT("Mobile.EnableUITextScaling")))
      {
         textScalingCvar->Set(1, ECVF_SetByDeviceProfile);
      }
   }
}

void FTAT::ShutdownModule()
{
#if WITH_GAMEPLAY_DEBUGGER
   if (IGameplayDebugger::IsAvailable())
   {
      IGameplayDebugger& gameplayDebuggerModule = IGameplayDebugger::Get();
      gameplayDebuggerModule.UnregisterCategory("Stealth");
      gameplayDebuggerModule.NotifyCategoriesChanged();
   }
#endif

   // NB: This hook sets a static variable, but is a non-static method for probably-oversight reasons
   if (auto* mesh = Cast<USkeletalMeshComponent>(USkeletalMeshComponent::StaticClass()->GetDefaultObject(false)))
   {
      mesh->UnregisterOnLODRequiredBonesUpdate(_skeletalMeshLodDelegate);
   }
}

IMPLEMENT_PRIMARY_GAME_MODULE( FTAT, TAT, "TAT" );
