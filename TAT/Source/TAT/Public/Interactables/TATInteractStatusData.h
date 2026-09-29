// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Common/TATGameplayTagTableRow.h"

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "TATInteractStatusData.generated.h"

class UPaperSprite;

// Data for mapping interact status tags to visualizations
USTRUCT(BlueprintType, meta=(RowNameTag=InteractStatusId))
struct FTATInteractStatusData : public FTATGameplayTagTableRow
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FGameplayTag InteractStatusId;

   /// Icon to show with the interaction input prompts
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayThumbnail = "true"))
   TSoftObjectPtr<UPaperSprite> Icon;

   // Color to tint the icon - NOTE: We should create a global style data asset and have the colors defined there instead.
   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FLinearColor IconColor { FLinearColor::White };
   
   /// If enabled, whenever this status id shows up on the UI, a warning that using it will break disguise will show up (if disguise is active)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (DisplayThumbnail = "true"))
   bool ShowBreaksDisguiseWarning = false;
};
