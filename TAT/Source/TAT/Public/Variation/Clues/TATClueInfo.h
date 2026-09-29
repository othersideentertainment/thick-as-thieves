// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueType.h"

// ue
#include "CoreMinimal.h"

#include "TATClueInfo.generated.h"

class UTATClueSpawnerComponent;
struct FTATClueContext;
enum class ETATClueType : uint8;

namespace TATClueInfoHelpers
{
   FString MakePreviewText(const FText& text, int32 maxLength = 48, TCHAR quoteChar = TEXT('\"'));
   FString FormatDebugDescription(FStringView clueType, FStringView description);
}

/// A base class for static data for a clue
USTRUCT()
struct TAT_API FTATClueInfo
{
   GENERATED_BODY()

   // Not actually needed, since nothing deletes via a pointer to base class (or indirectly), but silences warnings
   virtual ~FTATClueInfo() = default;

   virtual FTATClueBucketKey GetClueBucket() const;

   // CLUE-WIP: still not sure if push or pull makes most sense here
   virtual void ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const {}

   virtual FString GetDebugDescription() const { return FString(); }

#if WITH_EDITOR
   virtual void Validate(TFunctionRef<void (const FText&)> reportError) const {}
#endif
};
