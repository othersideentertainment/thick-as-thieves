// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "TATQuestRewardUtils.generated.h"

class UPaperSprite;
class UTATQuestRewardType;
struct FTATQuestReward;

UCLASS()
class TAT_API UTATQuestRewardUtils : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()
   
public:

   UFUNCTION(BlueprintPure, meta=(DisplayName = "Get Reward Icon", CompactNodeTitle="Icon", WorldContext = "worldContext"), Category = "Quest|Rewards")
   static TSoftObjectPtr<UPaperSprite> GetIconForRewardType(TSubclassOf<UTATQuestRewardType> rewardType, UObject* worldContext);
   UFUNCTION(BlueprintPure, meta=(DisplayName = "Get Reward Name", CompactNodeTitle = "Name", WorldContext = "worldContext"), Category="Quest|Rewards")
   static const FText& GetNameForRewardType(TSubclassOf<UTATQuestRewardType> rewardType, UObject* worldContext);

   UFUNCTION(BlueprintPure, meta=(DisplayName = "Get Reward Icon", CompactNodeTitle = "Icon", WorldContext = "worldContext"), Category="Quest|Rewards")
   static TSoftObjectPtr<UPaperSprite> GetIconForReward(const FTATQuestReward& reward, UObject* worldContext);
   UFUNCTION(BlueprintPure, meta=(DisplayName = "Get Reward Name", CompactNodeTitle = "Name", WorldContext = "worldContext"), Category="Quest|Rewards")
   static const FText& GetNameForReward(const FTATQuestReward& reward, UObject* worldContext);
};
