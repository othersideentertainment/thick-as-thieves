// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "PingSystem/OSEPingSystemInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPingSystemInfo)

// ue
#include "Misc/DataValidation.h"


DEFINE_LOG_CATEGORY(LogOSEPingSystem);

FOSEPingInfo::FOSEPingInfo()
   : Color(FLinearColor::Black)
   , TraceProfile(FName(TEXT("Ping")))
{
}

void UOSEPingSystemInfoAsset::FindPingInfoFromPingTag(FGameplayTag pingTag, FOSEPingInfo& outPingInfo, bool& found) const
{
   found = false;
   if (const FOSEPingInfo* foundInfo = FindPingInfoFromPingTag(pingTag))
   {
      found = true;
      outPingInfo = (*foundInfo);
   }
}

const FOSEPingInfo* UOSEPingSystemInfoAsset::FindPingInfoFromPingTag(const FGameplayTag& pingTag) const
{
   if (const FOSEPingInfo* foundPing = PingInfo.Find(pingTag))
   {
      return foundPing;
   }
   else
   {
      for (const FOSEPingPage& sprayPage : SprayPages)
      {
         if (const FOSEPingInfo* foundSpray = sprayPage.SprayInfo.Find(pingTag))
         {
            return foundSpray;
         }
      }
   }
   return nullptr;
}

void UOSEPingSystemInfoAsset::FindPingResponseInfoFromResponseTag(FGameplayTag responseTag, FOSEPingResponseInfo& outPingResponseInfo, bool& found) const
{
   found = false;
   if (const FOSEPingResponseInfo* foundInfo = FindPingResponseInfoFromResponseTag(responseTag))
   {
      found = true;
      outPingResponseInfo = (*foundInfo);
   }
}

const FOSEPingResponseInfo* UOSEPingSystemInfoAsset::FindPingResponseInfoFromResponseTag(const FGameplayTag& responseTag) const
{
   return PingResponseInfo.Find(responseTag);
}

#if WITH_EDITOR
EDataValidationResult UOSEPingSystemInfoAsset::IsDataValid(FDataValidationContext& context)
{
   // Make sure no tags are shared between pings and spray pages
   // Initialize with ping map, since keys are all unique
   TSet<FGameplayTag> pingTags;
   PingInfo.GetKeys(pingTags);

   TMap<const FGameplayTag, const FOSEPingInfo> uniqueSprayInfoTagMappings;

   // Validate each spray page
   for (int i = 0; i < SprayPages.Num(); i++)
   {
      const FOSEPingPage sprayPage = SprayPages[i];

      TSet<FGameplayTag> sprayTags;
      sprayPage.SprayInfo.GetKeys(sprayTags);
      
      for (const TPair<FGameplayTag, FOSEPingInfo>& sprayTagInfoPair : sprayPage.SprayInfo)
      {
         const FGameplayTag sprayTag = sprayTagInfoPair.Key;
         const FOSEPingInfo sprayInfo = *sprayPage.SprayInfo.Find(sprayTag);
         
         // Make sure spray tag isn't used by any pings
         if (pingTags.Contains(sprayTag))
         {
            const FOSEPingInfo pingInfo = *PingInfo.Find(sprayTag);
            context.AddError(FText::FromString(FString::Printf(TEXT("[%s] Spray %s (page %d) shares FGameplayTag %s with Ping %s"), *GetName(), *(sprayInfo.Name.ToString()), i+1, *(sprayTag.GetTagName().ToString()), *(pingInfo.Name.ToString()))));
         }

         // Make sure spray pages don't reuse tags
         if (const FOSEPingInfo* duplicateSprayInfo = uniqueSprayInfoTagMappings.Find(sprayTag))
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("[%s] Sprays %s (page %d) and %s share FGameplayTag %s"), *GetName(), *(sprayInfo.Name.ToString()), i+1, *duplicateSprayInfo->Name.ToString(), *(sprayTag.GetTagName().ToString()))));
         }
         else
         {
            uniqueSprayInfoTagMappings.Add(sprayTag, sprayInfo);
         }
      }
   }

   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

