// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Thiefsign/TATThiefsignSettings.h"

// tat
#include "Character/TATCharacterBase.h"

// ue
#include "Engine/AssetManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThiefsignSettings)
DEFINE_LOG_CATEGORY_STATIC(LogTATThiefsignSettings, Log, All);

bool UTATThiefsignSettings::AreSymbolsLoaded(bool startLoadIfUnloaded)
{
   if (SymbolsTable.IsValid())
   {
      return true;
   }
   else if (startLoadIfUnloaded && (!_loadingHandle.IsValid() || !_loadingHandle->IsLoadingInProgress()))
   {
      _loadingHandle = UAssetManager::GetStreamableManager().RequestAsyncLoad(SymbolsTable.ToSoftObjectPath(), [weakThis = MakeWeakObjectPtr(this)]()
      {
         if (weakThis.IsValid())
         {
            weakThis->_OnTableLoaded();
         }
      });
   }

   return false;
}

void UTATThiefsignSettings::CallOrRegisterSymbolsLoadedDelegate(const FSimpleMulticastDelegate::FDelegate& loadedDelegate)
{
   // In case the table was loaded in the time it took the callee to call this method, immediately fire the callback 
   if (SymbolsTable.IsValid())
   {
      loadedDelegate.Execute();
   }
   else
   {
      _onSymbolsLoadedDelegate.Add(loadedDelegate);
   }
}

void UTATThiefsignSettings::UnregisterSymbolsLoadedDelegates(const UObject* contextObject)
{
   _onSymbolsLoadedDelegate.RemoveAll(contextObject);
}

const FTATThiefsignInfo* UTATThiefsignSettings::FindThiefsignInfo(FGameplayTag thiefsignIdentifier)
{
   const bool loadSymbolsIfUnloaded = false;
   if (!AreSymbolsLoaded(loadSymbolsIfUnloaded))
   {
      return nullptr;
   }

   if (_thiefsignDataTableMap.Num() == 0)
   {
      _thiefsignDataTableMap.Reset(SymbolsTable.Get(), [](const FTATThiefsignInfo& thiefsignInfo) { return thiefsignInfo.Identifier; });
   }

   if (const FTATThiefsignInfo* row = _thiefsignDataTableMap.Find(SymbolsTable.Get(), thiefsignIdentifier))
   {
      constexpr const TCHAR* contextString = TEXT("UTATThiefsignSettings::FindThiefsignInfo");
      constexpr bool warnIfMissing = false;
      return row;
   }

   UE_LOG(LogTATThiefsignSettings, Error, TEXT("Failed to find %s Thiefsign identifier in UTATThiefsignSettings::FindThiefsignInfo!")
      , *thiefsignIdentifier.ToString());
   return nullptr;
}

void UTATThiefsignSettings::_OnTableLoaded()
{
   _onSymbolsLoadedDelegate.Broadcast();
   _onSymbolsLoadedDelegate.Clear();
}

const FTATThiefsignCharacterConfig& UTATThiefsignSettings::FindCharacterConfig(TSubclassOf<ATATCharacterBase> characterClass) const
{
   for (const auto& mapEntry : CharacterConfigs)
   {
      // characterClass contains a hard reference to the class we're looking for
      // Therefore, if a map entry class isn't loaded, it's not the one want
      if (mapEntry.Key.IsNull() || mapEntry.Key.IsPending())
      {
         continue;
      }

      if (characterClass->IsChildOf(mapEntry.Key.Get()))
      {
         return mapEntry.Value;
      }
   }

   // If no specific character config was found for the passed in class, fallback to the default
   return DefaultCharacterConfig;
}
