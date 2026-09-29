// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// TAT
#include "Common/TATGameplayTagTableRow.h"

// ue5
#include "GameplayTagContainer.h"

#include "TATPlayerExperience.generated.h"

USTRUCT(BlueprintType)
struct FTATFinishedMatchXPGained
{
   GENERATED_BODY()

   FTATFinishedMatchXPGained() = default;
   FTATFinishedMatchXPGained(int32 amountXP, FGameplayTag categoryTag);

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   int32 AmountXP = 0;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FGameplayTag CategoryTag;

   // for use by FindByKey
   bool operator==(const FGameplayTag& otherCategory) const { return otherCategory == CategoryTag; }
};

USTRUCT(BlueprintType)
struct FTATFinishedMatchXPGainedInfo
{
   GENERATED_BODY()

   FTATFinishedMatchXPGainedInfo() = default;
   FTATFinishedMatchXPGainedInfo(int32 amountXP, FText categoryText, FGameplayTag CategoryTag);

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   int32 AmountXP = 0;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FText CategoryText;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "XP"))
   FGameplayTag CategoryTag;
};

USTRUCT(BlueprintType)
struct TAT_API FTATXPGainInfo : public FTableRowBase
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = XP, meta = (Categories = "XP"))
   FGameplayTag Tag;

   UPROPERTY(EditAnywhere, Category = XP)
   FText DisplayName;
};

// A struct representing a player's progression experience
USTRUCT(BlueprintType)
struct FTATPlayerExperience
{
   GENERATED_BODY()

   FTATPlayerExperience() = default;
   FTATPlayerExperience(int32 newXP);

   UPROPERTY(BlueprintReadOnly)
   int32 XP = 0;

   UPROPERTY(BlueprintReadOnly)
   int32 Level = 1;

   UPROPERTY(BlueprintReadOnly)
   int32 CurrentLevelXP = 0;

   void SetXP(int32 totalXP);
   void AddXP(int32 amountXP);
   void FixLevelVersions();

private:
   void Update();
};

USTRUCT(BlueprintType, meta=(RowNameTag = "Tag"))
struct FTATPlayerStatInfo : public FTATGameplayTagTableRow
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(Categories = "PlayerStats"))
   FGameplayTag Tag;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText DisplayName;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta=(AllowedClasses="/Script/Engine.Texture,/Script/Engine.MaterialInterface,/Script/Engine.SlateTextureAtlasInterface", DisallowedClasses = "/Script/MediaAssets.MediaTexture"))
   TSoftObjectPtr<UObject> Icon;
};
