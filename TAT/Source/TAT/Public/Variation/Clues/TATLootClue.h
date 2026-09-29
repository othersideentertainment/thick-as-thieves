// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATLootClue.generated.h"

class UTATReadableClueWidget;

// Not a clue-type per se, but data that can be configured for a loot actor
// that makes it act like a readable clue on pickup
USTRUCT()
struct FTATLootClue
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, meta=(MultiLine))
   FText ClueText;

   // facts that the clue contains
   // Feeds into the clue journal and highlights
   UPROPERTY(EditAnywhere, meta=(RowType = "/Script/TAT.TATClueFactSpec", TitleProperty="{RowName}"))
   TArray<FDataTableRowHandle> Facts;

   // [Optional] The UI widget class used to display the clue
   // If not specified, it will use the default in project settings
   UPROPERTY(EditDefaultsOnly, Category=ReadableClue)
   TSoftClassPtr<UTATReadableClueWidget> ReadableWidgetOverride;

   void OnLootTaken(ACharacter* interactingCharacter) const;
#if WITH_EDITOR
   void Validate(TFunctionRef<void(const FText&)> reportError) const;
#endif
};
