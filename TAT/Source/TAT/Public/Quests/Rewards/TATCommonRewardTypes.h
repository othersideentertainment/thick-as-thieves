// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Loot/TATLootTypes.h"
#include "Quests/Rewards/TATQuestRewardType.h"

// ue5
#include "CoreMinimal.h"

#include "TATCommonRewardTypes.generated.h"

class UPaperSprite;
struct FTATQuestRewardContext;

UCLASS(meta = (DisplayName = "Money"))
class TAT_API UTATQuestRewardType_Money : public UTATQuestRewardType
{
   GENERATED_BODY()
   
public:

   virtual TSoftObjectPtr<UPaperSprite> GetIcon(const UObject* worldContext) const override;
   virtual const FText& GetName(const UObject* worldContext) const override;
   virtual void Grant(int32 quantity, const FTATQuestRewardContext& context) const override;
};


// Actually loot
UCLASS(Abstract, Blueprintable)
class TAT_API UTATQuestRewardType_Item : public UTATQuestRewardType
{
   GENERATED_BODY()
   
public:
   virtual TSoftObjectPtr<UPaperSprite> GetIcon(const UObject* worldContext) const override;
   virtual const FText& GetName(const UObject* worldContext) const override;
   virtual void Grant(int32 quantity, const FTATQuestRewardContext& context) const override;

protected:
   UPROPERTY(EditDefaultsOnly)
   FTATLootIdentifier ItemToGrant;
};
