// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Developer/TATOutfitSettings.h"

// tat
#include "CharacterCustomization/TATCharacterOutfits.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATOutfitSettings)

const FTATOutfitsMetadataTableRow* UTATOutfitSettings::FindOutfitMetadata(FGameplayTag outId, const UDataTable* outfitDataTable /*= nullptr*/) const
{
   if (outfitDataTable == nullptr)
   {
      outfitDataTable = _GetOutfitDataTable();
   }

   return outfitDataTable->FindRow<FTATOutfitsMetadataTableRow>(outId.GetTagName(), TEXT("FindOutfitMetadata"));
}

bool UTATOutfitSettings::BP_FindOutfitMetadata(FGameplayTag outfitID, FTATOutfitsMetadataTableRow& outfitMetadata)
{
   if (const FTATOutfitsMetadataTableRow* row = UTATOutfitSettings::Get().FindOutfitMetadata(outfitID))
   {
      outfitMetadata = *row;
      return true;
   }

   outfitMetadata = FTATOutfitsMetadataTableRow{};
   return false;
}

const UDataTable* UTATOutfitSettings::_GetOutfitDataTable() const
{
   UDataTable* outfitDataTable = OutfitMetadataTable.LoadSynchronous();
   check(outfitDataTable != nullptr);
   return outfitDataTable;
}
