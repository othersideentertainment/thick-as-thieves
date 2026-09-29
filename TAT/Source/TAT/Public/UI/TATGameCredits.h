// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "Engine/DataTable.h"

#include "TATGameCredits.generated.h"

UENUM(BlueprintType)
enum class ETATGameCreditType : uint8
{
   Name = 0,
   Section    UMETA(ToolTip = "General purpose header row. Eg. department or category"),
   Company    UMETA(ToolTip = "Larger header row to indicate what company a credit belongs to"),
   Custom     UMETA(ToolTip = "Custom header row. Can be used with CustomWidget to show anything (eg. one or more logos)"),
};

class UTATGameCreditRowWidget;

USTRUCT(BlueprintType)
struct TAT_API FTATGameCredit : public FTableRowBase
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Credits)
   ETATGameCreditType Type = ETATGameCreditType::Name;

   /// Only enabled rows are shown on the credits screen
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Credits)
   bool Enabled = true;
   
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Credits, Meta = (EditCondition = "Type != ETATGameCreditType::Name", EditConditionHides))
   FText PrimaryText;

   /// Optional second line of text that can be shown under the primary one
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Credits, Meta = (EditCondition = "Type != ETATGameCreditType::Name", EditConditionHides))
   FText SecondaryText;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Credits, Meta = (EditCondition = "Type == ETATGameCreditType::Name", EditConditionHides))
   FText Name;

   /// Optional second line of text that can be shown under the name
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Credits, Meta = (EditCondition = "Type == ETATGameCreditType::Name", EditConditionHides))
   FText Role;

   /// A custom UMG widget to use for this row.
   /// Can be used for any kind of content (eg. one or more logos)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Credits)
   TSoftClassPtr<UTATGameCreditRowWidget> CustomWidget;
};


UCLASS()
class TAT_API UTATGameCreditHelpers : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()
   
public:
   UFUNCTION(BlueprintCallable, Category = Credits)
   static void GetAllGameCreditRowNames(UDataTable* creditsDataTable, TArray<FName>& rowNames);

};
