// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Engine/Console.h"

#include "TATConsole.generated.h"

UCLASS()
class TAT_API UTATConsole : public UConsole
{
   GENERATED_BODY()
   
public:
   // from UConsole
   virtual void AugmentRuntimeAutoCompleteList(TArray<FAutoCompleteCommand>& list) override;

private:
   FAutoCompleteCommand _GenerateFuncAutoComplete(const UFunction* func, bool isServerExec);
};
