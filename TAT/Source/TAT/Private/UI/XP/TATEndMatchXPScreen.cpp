// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/XP/TATEndMatchXPScreen.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "UI/Queue/TATUIQueue.h"
#include "Progression/TATProgressionSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEndMatchXPScreen)

DEFINE_LOG_CATEGORY_STATIC(LogTATEndMatchXPScreen, Log, All);

UTATUIQueueAction* UTATEndMatchXPScreen::CreateAction(const FMatchPersistentXPGainedData& persistentXPGainedData)
{
   return UTATScreenQueueAction::Create(UTATProjectSettings::Get().EndMatchXPScreen,
      [persistentXPGainedData = persistentXPGainedData](UTATScreenWidget* screen) mutable
      {
         UTATEndMatchXPScreen* xpScreen = CastChecked<UTATEndMatchXPScreen>(screen);
         if (xpScreen)
         {
            xpScreen->CurrentLevel = persistentXPGainedData.LevelBeforeXPGain;
            xpScreen->CurrentXP = persistentXPGainedData.LevelXPBeforeXPGain;

            // right now the XPDataTable is only used for this Screen, so we load here and it will be unload by the engine GC later
            if (!UTATProgressionSettings::Get().XPDataTable.IsNull())
            {
               UTATProgressionSettings::Get().XPDataTable.LoadSynchronous();
               const UDataTable* xpDataTable = UTATProgressionSettings::Get().XPDataTable.Get();

               for (const FTATFinishedMatchXPGained& xpGained : persistentXPGainedData.XPGainedArray)
               {
                  const FTATXPGainInfo *info = xpDataTable->FindRow<FTATXPGainInfo>(xpGained.CategoryTag.GetTagName(), TEXT("UTATEndMatchXPScreen::CreateAction"));
                  if (info)
                  {
                     xpScreen->EndMatchXPGained.Add(FTATFinishedMatchXPGainedInfo(xpGained.AmountXP, info->DisplayName, info->Tag));
                  }
                  else
                  {
                     UE_LOG(LogTATEndMatchXPScreen, Error, TEXT("UTATEndMatchXPScreen::CreateAction -> Could not find %s tag in XPDataTable of UTATProgressionSettings."), *xpGained.CategoryTag.GetTagName().ToString());
                  }
               }
            }
            else
            {
               UE_LOG(LogTATEndMatchXPScreen, Error, TEXT("UTATEndMatchXPScreen::CreateAction -> XPDataTable of UTATProgressionSettings is null."));
            }
         }
      });
}
