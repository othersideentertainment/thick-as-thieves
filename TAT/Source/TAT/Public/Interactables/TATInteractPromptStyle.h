// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Common/TATGameplayTagTableRow.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "PaperSprite.h"
#include "Engine/DataTable.h"

#include "TATInteractPromptStyle.generated.h"

/// The designer-oriented data related to interact prompts
USTRUCT(BlueprintType, meta=(RowNameTag=InteractActionTag))
struct TAT_API FTATInteractPromptStyle : public FTATGameplayTagTableRow
{
   GENERATED_BODY()

public:
   // Identifying tag for Interact Prompts to associate with this Style
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Meta = (Categories = "InteractAction"))
   FGameplayTag InteractActionTag;

   // Icon to appear next to the Interact Prompt Action
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Style")
   TObjectPtr<UPaperSprite> Glyph;

   // Override for the size of the Interact Prompt.  A size of 0 means no override occurs and defaults to the widgets default size
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Style")
   int32 FontSize = 0;
};
