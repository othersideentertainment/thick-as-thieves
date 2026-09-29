// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Engine/DataAsset.h"
#include "Math/Color.h"
#include "GameplayTagContainer.h"

#include "TATUIColorsData.generated.h"

// Defines a semantic color palette (eg. Success, Error, Caution, Processing, etc)
USTRUCT()
struct TAT_API FTATUISemanticPaletteColorEntry
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   FLinearColor LightColor = FLinearColor::Transparent;

   UPROPERTY(EditDefaultsOnly)
   FLinearColor DarkColor = FLinearColor::Transparent;
};

// Data asset used to store standard color data for use across various UI elements
UCLASS()
class TAT_API UTATUIColorsData : public UDataAsset
{
   GENERATED_BODY()

   // from UObject
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

public:

   UPROPERTY(EditDefaultsOnly, meta = (Categories = "UI.ColorPalettes"))
   TMap<FGameplayTag, FTATUISemanticPaletteColorEntry> SemanticColorPalettes;
};
