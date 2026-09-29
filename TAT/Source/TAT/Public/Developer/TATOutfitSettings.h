// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATDataTableMap.h"

// ue5
#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "GameplayTagContainer.h"

#include "TATOutfitSettings.generated.h"

struct FTATOutfitsMetadataTableRow;
class UDataTable;

/**
 * 
 */
UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Outfit Settings"))
class TAT_API UTATOutfitSettings : public UDeveloperSettings
{
	GENERATED_BODY()
	
public:
   static const UTATOutfitSettings& Get() { return *GetDefault<UTATOutfitSettings>(); }

   const FTATOutfitsMetadataTableRow* FindOutfitMetadata(FGameplayTag outId, const UDataTable* outfitDataTable = nullptr) const;

   UFUNCTION(BlueprintCallable, DisplayName = "Find Outfit Metadata", Category = "Outfit Settings")
   static bool BP_FindOutfitMetadata(FGameplayTag outfitID, FTATOutfitsMetadataTableRow& outfitMetadata);

   UPROPERTY(Config, EditAnywhere, Category = "Outfit", Meta=(RowType="/Script/TAT.TATOutfitMetadataTableRow"))
   TSoftObjectPtr<UDataTable> OutfitMetadataTable;

private:
   const UDataTable* _GetOutfitDataTable() const;
};
