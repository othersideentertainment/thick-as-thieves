// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

/**
 * 
 */
class OSEBUGREPORTER_API FOSEBugSubmitter
{
public:
   static void SubmitBug(const TArray<FString>& Args, UWorld*);
   static FString GetAttributesString(UWorld* world, class AOSECharacterBase* character);
};
