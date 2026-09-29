// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATQuestFormatParamSource.generated.h"

// Base class for polymorphic struct to produce dynamic clue format params in missions
USTRUCT()
struct TAT_API FTATQuestFormatParamSource
{
   GENERATED_BODY()

   struct FParams
   {
      int32 MapSeed = 0;
   };

   virtual ~FTATQuestFormatParamSource() = default;

   virtual void AddTextReplacement(const FParams& params, TFunctionRef<void (const FString&, const FText&)> addFormatParam) const {}

#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const {}
#endif
};
