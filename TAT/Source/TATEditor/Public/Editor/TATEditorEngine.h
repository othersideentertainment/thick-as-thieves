// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Editor/EditorEngine.h"
#include "TATEditorEngine.generated.h"


UCLASS()
class TATEDITOR_API UTATEditorEngine : public UEditorEngine
{
   GENERATED_BODY()

public:

   //	Game-specific function called per-actor by Map_Check
   //
   //	@param	Str						The exec command parameters
   //	@param	Ar						The output archive for logging (?)
   //	@param	bCheckDeprecatedOnly	If true, only check for deprecated classes
   virtual bool Game_Map_Check_Actor(const TCHAR * str, FOutputDevice & ar, bool checkDeprecatedOnly, AActor * inActor) override;


   //	Game-specific function called by Map_Check BEFORE iterating over all actors.
   //
   //	@param	Str						The exec command parameters
   //	@param	Ar						The output archive for logging (?)
   //	@param	bCheckDeprecatedOnly	If true, only check for deprecated classes
   virtual bool Game_Map_Check(UWorld* inWorld, const TCHAR* str, FOutputDevice& ar, bool checkDeprecatedOnly) override;
	
};
