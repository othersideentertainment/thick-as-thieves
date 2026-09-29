// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Settings/TATEnhancedInputUserSettings.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEnhancedInputUserSettings)

bool UTATEnhancedInputUserSettings::IsDirty() const
{
   for (const TPair<FGameplayTag, UEnhancedPlayerMappableKeyProfile*> profilePair : SavedKeyProfiles)
   {
			for (const TPair<FName, FKeyMappingRow>& mappingRow : profilePair.Value->GetPlayerMappingRows())
         {
            for (const FPlayerKeyMapping& mapping : mappingRow.Value.Mappings)
            {
               // We want to save any dirty or customized key mappings
               if (mapping.IsDirty())
               {
                  return true;
               }
            }
         }
   }
   return false;
}

void UTATEnhancedInputUserSettings::ApplySettings()
{
   SaveSettings();
   Super::ApplySettings();
   StoreInitialSettings();
}

void UTATEnhancedInputUserSettings::StoreInitialSettings()
{
   // This will only allow keyboard inputs (bMatchBasicKeyTypes checks against the key provided)
   FPlayerMappableKeyQueryOptions queryOptions;
   queryOptions.bMatchBasicKeyTypes = true;
   queryOptions.KeyToMatch = EKeys::W;
   
   if (const UEnhancedPlayerMappableKeyProfile* Profile = GetCurrentKeyProfile())
   {
      for(const TPair<FName, FKeyMappingRow>& row : Profile->GetPlayerMappingRows())
      {
         TMap<EPlayerMappableKeySlot, FKey> rowMap = TMap<EPlayerMappableKeySlot, FKey>();
         for (const FPlayerKeyMapping& Mapping : row.Value.Mappings)
         {
            if (Profile->DoesMappingPassQueryOptions(Mapping, queryOptions))
            {
               rowMap.Add(Mapping.GetSlot(), Mapping.GetCurrentKey());
            }
         }
         _InitialKeyMappings.Add(row.Key, rowMap);
      }
   }
}

void UTATEnhancedInputUserSettings::RestoreInitialSettings()
{
   for (const TPair<FName, TMap<EPlayerMappableKeySlot, FKey>>& mapping : _InitialKeyMappings)
   {
      for (const TPair<EPlayerMappableKeySlot, FKey>& pair : mapping.Value)
      {
         FMapPlayerKeyArgs args = {};
         args.MappingName = mapping.Key;
         args.Slot = static_cast<EPlayerMappableKeySlot>(static_cast<uint8>(pair.Key));
         args.NewKey = pair.Value;
         FGameplayTagContainer FailureReason;
         MapPlayerKey(args, FailureReason);
      }
   }
   
   // To reset the "IsDirty" flag either we modify the engine and expose this class as friendly OR
   SaveSettings();
   OnSettingsChanged.Broadcast(this);
}
