// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

#include "TATDifficulty.generated.h"

UENUM(BlueprintType)
enum class ETATDifficulty : uint8
{
   Easy = 0 UMETA(DisplayName="Novice"),
   Normal = 1 UMETA(DisplayName="Thief"),
   Hard = 2 UMETA(DisplayName="Master Thief"),
   MAX UMETA(Hidden)
};
ENUM_RANGE_BY_COUNT(ETATDifficulty, ETATDifficulty::MAX);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_MatchSetting_Difficulty);

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Difficulty_Easy);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Difficulty_Normal);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Difficulty_Hard);

namespace TATDifficulty
{
   TAT_API FGameplayTag GetDifficultyTag(ETATDifficulty difficulty);
   ETATDifficulty GetDifficultyForMatch(UWorld* world);
   void InitDifficulty(UWorld* world);
}
