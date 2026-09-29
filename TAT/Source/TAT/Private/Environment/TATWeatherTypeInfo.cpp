// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATWeatherTypeInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWeatherTypeInfo)

DEFINE_LOG_CATEGORY_STATIC(LogTATWeatherTypeInfo, Log, All);

bool FTATWeatherTypeInfo::IsWeatherTypeAllowedInLevel(UWorld* world) const
{
   switch (LevelFilterMode)
   {
   case ETATWeatherLevelFilter::AllowAll:
      return true;

   case ETATWeatherLevelFilter::AllowNone:
      return false;

   case ETATWeatherLevelFilter::UseAllowList:
      for (const TSoftObjectPtr<UWorld>& level : LevelFilterList)
      {
         // Assume that the level we're checking against is already loaded - the last thing we want is to *load* all the levels in the whitelist
         // just to check if an already loaded level is in the list.
         if (!level.IsValid())
         {
            continue;
         }

         if (world == level.Get())
         {
            return true;
         }
      }
      return false;

   case ETATWeatherLevelFilter::UseDenyList:
      for (const TSoftObjectPtr<UWorld>& level : LevelFilterList)
      {
         // Assume that the level we're checking against is already loaded - the last thing we want is to *load* all the levels in the whitelist
         // just to check if an already loaded level is in the list.
         if (!level.IsValid())
         {
            continue;
         }

         if (world == level.Get())
         {
            return false;
         }
      }
      return true;

   default:
      checkNoEntry();
      break;
   }

   return false;
}

bool FTATWeatherTypeInfo::IsWeatherTypeAllowedInLevel(const TSoftObjectPtr<UWorld>& world) const
{
   switch (LevelFilterMode)
   {
   case ETATWeatherLevelFilter::AllowAll:
      return true;
   case ETATWeatherLevelFilter::AllowNone:
      return false;
   case ETATWeatherLevelFilter::UseAllowList:
      return LevelFilterList.Contains(world);
   case ETATWeatherLevelFilter::UseDenyList:
      return !LevelFilterList.Contains(world);
   default:
      checkNoEntry();
      break;
   }
   return false;
}
