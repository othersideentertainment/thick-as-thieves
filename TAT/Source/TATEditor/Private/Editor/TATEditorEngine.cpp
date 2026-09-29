// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Editor/TATEditorEngine.h"

// tat
#include "Editor/TATCommonMapCheck.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEditorEngine)

bool UTATEditorEngine::Game_Map_Check_Actor(const TCHAR* str, FOutputDevice& ar, bool checkDeprecatedOnly, AActor* inActor)
{
   Super::Game_Map_Check_Actor(str, ar, checkDeprecatedOnly, inActor);

   if(!checkDeprecatedOnly)
   {
      TATCommonMapCheck::CheckActor(inActor);
   }

   return true;
}

bool UTATEditorEngine::Game_Map_Check(UWorld* inWorld, const TCHAR* str, FOutputDevice& ar, bool checkDeprecatedOnly)
{
   Super::Game_Map_Check(inWorld, str, ar, checkDeprecatedOnly);

   if (!checkDeprecatedOnly)
   {
      TATCommonMapCheck::CheckWorld(inWorld);
   }

   return true;
}
