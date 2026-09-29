// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/Rewards/TATCommonRewardTypes.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Developer/TATLootSettings.h"
#include "Quests/Rewards/TATQuestRewardContext.h"
#include "SaveGame/TATSaveGame.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCommonRewardTypes)

TSoftObjectPtr<UPaperSprite> UTATQuestRewardType_Money::GetIcon(const UObject* worldContext) const
{
   return UTATProjectSettings::Get().MoneyReward.Icon;
}

const FText& UTATQuestRewardType_Money::GetName(const UObject* worldContext) const
{
   return UTATProjectSettings::Get().MoneyReward.Name;
}

void UTATQuestRewardType_Money::Grant(int32 quantity, const FTATQuestRewardContext& context) const
{
   check(context.IsValid());
   context.SaveGame->UpdateMoney(quantity);
}

TSoftObjectPtr<UPaperSprite> UTATQuestRewardType_Item::GetIcon(const UObject* worldContext) const
{
   if (const FTATLootInfo* lootInfo = UTATLootSettings::Get().GetLootInfo(worldContext, ItemToGrant))
   {
      return lootInfo->DisplaySprite;
   }
   else
   {
      return TSoftObjectPtr<UPaperSprite>();
   }
}

const FText& UTATQuestRewardType_Item::GetName(const UObject* worldContext) const
{
   if (const FTATLootInfo* lootInfo = UTATLootSettings::Get().GetLootInfo(worldContext, ItemToGrant))
   {
      return lootInfo->DisplayName;
   }
   else
   {
      return FText::GetEmpty();
   }
}

void UTATQuestRewardType_Item::Grant(int32 quantity, const FTATQuestRewardContext& context) const
{
   check(context.IsValid());
   UTATSaveGame* save = context.SaveGame.Get();

   FTATSavedLootAddRequest addRequest;
   addRequest.AddCount(ItemToGrant, quantity, save);
   save->AddLoot(context.Character, addRequest);
}
