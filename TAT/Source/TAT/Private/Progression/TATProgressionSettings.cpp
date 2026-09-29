// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Progression/TATProgressionSettings.h"

// TAT
#include "Progression/TATPlayerExperience.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATProgressionSettings)

UTATProgressionSettings::UTATProgressionSettings()
{
   UpgradeEditorProgressionNodeStyles[static_cast<int32>(ETATUpgradeProgressionType::None)] = {
      EOSEGenericGraphNodeStyle::BorderDark,
      FLinearColor(0.500000f, 0.500000f, 0.500000f, 1.000000f),
   };
   UpgradeEditorProgressionNodeStyles[static_cast<int32>(ETATUpgradeProgressionType::MinorUpgrade)] = {
      EOSEGenericGraphNodeStyle::Flat,
      FLinearColor(0.178403f, 0.461299f, 1.000000f, 1.000000f),
   };
   UpgradeEditorProgressionNodeStyles[static_cast<int32>(ETATUpgradeProgressionType::MajorUpgrade)] = {
      EOSEGenericGraphNodeStyle::BorderLight,
      FLinearColor(0.037109f, 0.395833f, 0.238891f, 1.000000f),
   };
   UpgradeEditorProgressionNodeStyles[static_cast<int32>(ETATUpgradeProgressionType::TierUpgrade)] = {
      EOSEGenericGraphNodeStyle::Glossy,
      FLinearColor(0.566993f, 0.000000f, 1.000000f, 1.000000f),
   };
}

const UTATProgressionSettings& UTATProgressionSettings::Get()
{
   return *GetDefault<UTATProgressionSettings>();
}

const UTATProgressionSettings* UTATProgressionSettings::BP_GetProgressionSettings()
{
   return GetDefault<UTATProgressionSettings>();
}

bool UTATProgressionSettings::GetPlayerStatInfo(FGameplayTag statTag, FTATPlayerStatInfo& statInfo)
{
   const UTATProgressionSettings* settings = GetDefault<UTATProgressionSettings>();
   if (!settings)
   {
      return false;
   }

   const UDataTable* dataTable = settings->PlayerStatDataTable.LoadSynchronous();
   if (!dataTable)
   {
      return false;
   }

   const FTATPlayerStatInfo* row = dataTable->FindRow<FTATPlayerStatInfo>(statTag.GetTagName(), TEXT("GetPlayerStatInfo"));
   if (!row)
   {
      return false;
   }

   statInfo = *row;
   return true;
}

int32 UTATProgressionSettings::GetXPForNextLevel(const int level)
{
   const UTATProgressionSettings* settings = GetDefault<UTATProgressionSettings>();
   if (settings == nullptr)
      return 0;
   if (level <= 0)
      return 0;
   return settings->XPToLevel.GetValueAtLevel(level);
}

int32 UTATProgressionSettings::GetTotalXPRequiredForLevel(const int level)
{
   int xp = 0;
   for (int i = 0; i < level; i++)
   {
      xp += GetXPForNextLevel(i);
   }
   return xp;
}
