// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Character/TATCharacterMetadata.h"
#include "Items/TATInventoryTypes.h"

// ose
#include "Abilities/OSEUpgradeState.h"

// ue4
#include "Engine/DeveloperSettings.h"

#include "TATPlayerInventorySettings.generated.h"

class UTATItemInfo;

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Player Inventory Settings"))
class TAT_API UTATPlayerInventorySettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   static const UTATPlayerInventorySettings& Get() { return *GetDefault<UTATPlayerInventorySettings>(); }

   FTATInventorySize GetSizesForCharacter(ETATCharacter character, const FUpgradeState& upgradeState) const;

   UPROPERTY(Config, EditAnywhere, Category = "Inventory Sizes")
   TMap<ETATCharacter, FTATInventorySize> SizeByCharacter;


   UPROPERTY(Config, EditAnywhere, Category = "Item Prompts")
   FText PromptInventoryFull;

   UPROPERTY(Config, EditAnywhere, Category = "Item Prompts")
   FText PromptTakeOne;

   UPROPERTY(Config, EditAnywhere, Category = "Item Prompts")
   FText PromptTakeMultiple;
};
