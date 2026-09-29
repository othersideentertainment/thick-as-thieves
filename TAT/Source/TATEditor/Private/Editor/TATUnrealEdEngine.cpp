// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Editor/TATUnrealEdEngine.h"

// ue
#include "Developer/SourceControl/Public/ISourceControlModule.h"
#include "Developer/SourceControl/Public/ISourceControlProvider.h"

// tat
#include "Editor/TATCommonMapCheck.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATUnrealEdEngine)

void UTATUnrealEdEngine::Init(IEngineLoop* InEngineLoop)
{
   Super::Init(InEngineLoop);

   auto& sourceControlModule = ISourceControlModule::Get();
   if (sourceControlModule.IsEnabled())
   {
      ISourceControlProvider& sourceControlProvider = sourceControlModule.GetProvider();
      if (sourceControlProvider.GetName() == "Perforce")
      {
         // TODO don't hardcode this
         TArray<FString> branchNames = { "//TAT/tat_release", "//TAT/main" };
         sourceControlProvider.RegisterStateBranches(branchNames, "TAT/Content");
      }
   }
}

bool UTATUnrealEdEngine::Game_Map_Check_Actor(const TCHAR* str, FOutputDevice& ar, bool checkDeprecatedOnly, AActor* inActor)
{
   Super::Game_Map_Check_Actor(str, ar, checkDeprecatedOnly, inActor);

   if (!checkDeprecatedOnly)
   {
      TATCommonMapCheck::CheckActor(inActor);
   }

   return true;
}

bool UTATUnrealEdEngine::Game_Map_Check(UWorld* inWorld, const TCHAR* str, FOutputDevice& ar, bool checkDeprecatedOnly)
{
   Super::Game_Map_Check(inWorld, str, ar, checkDeprecatedOnly);

   if (!checkDeprecatedOnly)
   {
      TATCommonMapCheck::CheckWorld(inWorld);
   }

   return true;
}
