// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "Blueprint/UserWidget.h"

#include "TATGameCreditRowWidget.generated.h"

struct FTATGameCredit;

UCLASS(Blueprintable)
class TAT_API UTATGameCreditRowWidget : public UUserWidget
{
   GENERATED_BODY()

public:
   /// The primary text of the row (eg. a person's name, or the name of a department or company)
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Credits, Meta = (ExposeOnSpawn = true))
   FText PrimaryText;

   /// Optional: The secondary text of the row (eg. a person's role). Should not be shown if empty.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Credits, Meta = (ExposeOnSpawn = true))
   FText SecondaryText;

   UFUNCTION(BlueprintCallable, BlueprintNativeEvent, Category = Credits)
   void OnCreatedCreditRow(const FTATGameCredit& credit);

};
