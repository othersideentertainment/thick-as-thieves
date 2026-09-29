// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Settings/TATMatchSettingsPropertyDef.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMatchSettingsPropertyDef)

const FTATMatchSettingsQueryContext FTATMatchSettingsQueryContext::NullContext{};

// static
FTATMatchSettingsQueryContext FTATMatchSettingsQueryContext::MakeFromWorldContext(const UObject* contextObject)
{
   FTATMatchSettingsQueryContext result{};
   result.Map = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull);
   result.WorldContextObject = contextObject;
   return result;
}

const UTATMatchSettingsGameplayTagQuery* FTATMatchSettingsGameplayTagGroup::GetCustomQueryObject() const
{
   if (!CustomQuery)
   {
      return nullptr;
   }
   UTATMatchSettingsGameplayTagQuery* tagQueryCDO = CustomQuery->GetDefaultObject<UTATMatchSettingsGameplayTagQuery>();
   check(tagQueryCDO != nullptr);
   return tagQueryCDO;
}

void FTATMatchSettingsGameplayTagGroup::GetAllGameplayTags(TArray<FTATMatchSettingsGameplayTag>& outTags, const FTATMatchSettingsQueryContext& queryContext, bool includeNone) const
{
   outTags.Reset();
   if (includeNone && AllowNone)
   {
      outTags.Add(FTATMatchSettingsGameplayTag{ FGameplayTag::EmptyTag, NoneLabel });
   }
   if (const UTATMatchSettingsGameplayTagQuery* queryObject = GetCustomQueryObject())
   {
      queryObject->GetGameplayTags(queryContext, outTags);
      outTags.Append(GameplayTags);
   }
   else
   {
      outTags = GameplayTags;
   }
}

bool FTATMatchSettingsGameplayTagGroup::ContainsGameplayTag(FGameplayTag gameplayTag, const FTATMatchSettingsQueryContext& queryContext) const
{
   if (!gameplayTag.IsValid())
   {
      return AllowNone;
   }
   if (const UTATMatchSettingsGameplayTagQuery* queryObject = GetCustomQueryObject())
   {
      if (queryObject->ContainsGameplayTag(queryContext, gameplayTag))
      {
         return true;
      }
   }
   for (const FTATMatchSettingsGameplayTag& tag : GameplayTags)
   {
      if (tag.Tag == gameplayTag)
      {
         return true;
      }
   }
   return false;
}

FString FTATMatchSettingsGameplayTagGroup::ToTagListDebugString(const TCHAR* separator) const
{
   TStringBuilder<255> tagList;
   int32 tagCount = 0;
   if (AllowNone)
   {
      tagList.Append(TEXT("None"));
      ++tagCount;
   }
   for (const FTATMatchSettingsGameplayTag& tag : GameplayTags)
   {
      if (tagCount > 0)
      {
         tagList.Append(separator);
      }
      tagList.Append(tag.Tag.ToString());
      ++tagCount;
   }
   return FString(tagList);
}
