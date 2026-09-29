// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATClueTextUtils.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Loot/TATLootTags.h"
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/Clues/TATClueLocationInterface.h"
#include "Variation/Clues/TATClueSpawnUtils.h"
#include "Quests/TATQuestDataSubsystem.h"
#include "Quests/TATQuestObjective.h"
#include "Quests/TATQuestTags.h"


FText TATClueTextUtils::FindItemName(FGameplayTag sourceTag, UObject* worldContext)
{
   if(sourceTag.MatchesTag(TAG_Contract))
   {
      const FTATQuestObjectiveInfo* objective = UTATQuestDataSubsystem::Get(worldContext).FindContractObjective(sourceTag);
      // TODO: Wean ourselves off uses of the singular form of GetRelatedLootTag (and possibly this lookup altogether)
      const FGameplayTag itemTag = objective ? objective->GetRelatedLootTag() : FGameplayTag();
      if(itemTag.IsValid() && itemTag != sourceTag)
      {
         // At least it isn't a simple cycle
         checkNoRecursion();
         return FindItemName(itemTag, worldContext);
      }
   }
   else if(sourceTag.MatchesTag(TAG_Loot))
   {
      if(const FTATLootInfo* lootInfo = UTATLootSettings::Get().GetLootInfo(worldContext, FTATLootIdentifier(sourceTag)))
      {
         return lootInfo->DisplayName;
      }
   }

   return FText();
}

void TATClueTextUtils::AddItemNameToParams(FTATClueFormatParams& params, FGameplayTag sourceTag, UObject* worldContext)
{
   FText itemName = FindItemName(sourceTag, worldContext);
   if (!itemName.IsEmpty())
   {
      params.Add(TEXT("Item"), MoveTemp(itemName));
   }
}

FText TATClueTextUtils::FormatUsingContext(const FText& format, const FTATClueContext& clueContext, UObject* worldContext)
{
   FTATClueTextFormatContext formatContext;
   if (const ITATClueLocationInterface* location = clueContext.Location.Get())
   {
      formatContext.LocationName = location->GetClueLocationName();
   }
   formatContext.SourceTag = clueContext.SourceTag;
   formatContext.ExtraFormatParams = clueContext.ExtraFormatParams.Get();

   return FormatUsingContext(format, formatContext, worldContext);
}

FText TATClueTextUtils::FormatUsingContext(const FText& format, const FTATClueTextFormatContext& context, UObject* worldContext)
{
   FTextFormat textFormat(format);
      
   // since replicated text can be big, do the work to only use the arguments in the format
   TArray<FString> argumentNames;
   textFormat.GetFormatArgumentNames(argumentNames);
   if (argumentNames.IsEmpty())
   {
      return format;
   }
      
   FFormatNamedArguments arguments;

   // Explicit text params can take priority unless we decide otherwise
   if (const FTATClueFormatParams* extraParams = context.ExtraFormatParams)
   {
      argumentNames.RemoveAll([extraParams, &arguments] (const FString& parameterName)
      {
         if(const FText* value = extraParams->Find(parameterName))
         {
            arguments.Add(parameterName, *value);
            return true;
         }

         return false;
      });
   }
   
   if (const int foundIndex = argumentNames.IndexOfByKey(TEXTVIEW("Location"));
       foundIndex >= 0)
   {
      arguments.Add(TEXT("Location"), context.LocationName);
      argumentNames.RemoveAtSwap(foundIndex);
   }
   if (const int foundIndex = argumentNames.IndexOfByKey(TEXTVIEW("Item"));
       foundIndex >= 0)
   {
      arguments.Add(TEXT("Item"), TATClueTextUtils::FindItemName(context.SourceTag, worldContext));
      argumentNames.RemoveAtSwap(foundIndex);
   }

#if !NO_LOGGING
   // Log missing parameters
   // Could eventually be complemented by validation of the parameters use by clue sets by their referrer
   for(const FString& parameterName : argumentNames)
   {
      UE_LOG(LogTATMapVariation, Warning, TEXT("Unhandled text replacement {%s} in clue (Location: '%s', Source:'%s', Text:'%s')"),
         *parameterName, *context.LocationName.ToString(), *context.SourceTag.ToString(), *format.ToString());
   }
#endif
   
   return FText::Format(MoveTemp(textFormat), arguments);
}
