// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "TATUpgradeCurrencyMetadata.generated.h"

class UPaperSprite;

UENUM(BlueprintType)
enum class ETATCurrencyCategory : uint8
{
   Money,
   UpgradeCurrency,
};

USTRUCT(BlueprintType)
struct TAT_API FTATCurrencyMetadata
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency Metadata")
   ETATCurrencyCategory Category = ETATCurrencyCategory::UpgradeCurrency;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency Metadata", Meta=(Categories = "UpgradeCurrency", EditCondition = "Category == ETATCurrencyCategory::UpgradeCurrency"))
   FGameplayTag Tag;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency Metadata")
   FText LocalizedName;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency Metadata")
   FText Description;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency Metadata")
   TSoftObjectPtr<UPaperSprite> Icon;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Currency Metadata")
   FLinearColor Color = FLinearColor::White;

   FTATCurrencyMetadata() = default;
   explicit FTATCurrencyMetadata(ETATCurrencyCategory category) : Category(category) {}

   bool operator==(const FGameplayTag& otherTag) const { return Tag == otherTag; }
};

UCLASS(BlueprintType)
class TAT_API UTATUpgradeCurrencyMetadataAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintPure, Category = "Upgrade Currency Metadata")
   const FTATCurrencyMetadata& GetMoneyCurrencyMetadata() const { return MoneyMetadata; }

   const FTATCurrencyMetadata& GetUpgradeCurrencyMetadata(FGameplayTag currencyTag) const;

   UFUNCTION(BlueprintPure, DisplayName = "Get Upgrade Currency Metadata", Category = "Upgrade Currency Metadata")
   bool BP_GetUpgradeCurrencyMetadata(FGameplayTag currencyTag, FTATCurrencyMetadata& currencyMetadata) const;

   UFUNCTION(BlueprintPure, Category = "Upgrade Currency Metadata")
   const TArray<FTATCurrencyMetadata>& GetAllUpgradeCurrencyMetadata() const { return _upgradeCurrencies; }

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

private:
   UPROPERTY(EditDefaultsOnly)
   FTATCurrencyMetadata MoneyMetadata{ ETATCurrencyCategory::Money };

   // reason for keeping it in array instead of a map: so there is a consistent order in case something wants that

   UPROPERTY(EditDefaultsOnly, meta = (TitleProperty="LocalizedName"))
   TArray<FTATCurrencyMetadata> _upgradeCurrencies;
   
};
