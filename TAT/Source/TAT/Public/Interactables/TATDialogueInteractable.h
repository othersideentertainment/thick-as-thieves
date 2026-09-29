// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Interactables/TATUIInteractable.h"

#include "TATDialogueInteractable.generated.h"

class ATATPlayerController;
class UTATMissionsMetadataAsset;
class UTATScreenWidget;

UCLASS(Blueprintable, BlueprintType)
class TAT_API ATATDialogueInteractable : public ATATUIInteractable
{
   GENERATED_BODY()
   
public:
   // Default text to display in the dialgue UI.
   // NOTE: this is REQUIRED to be in a string table, you cannot just type text into this field, it's too easy for these strings to need to re-localize when they live in the map
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "TAT|Dialogue")
   FText DefaultDialogueText;

protected:
#if WITH_EDITOR
   // from UObject
   virtual void CheckForErrors() override;
#endif // WITH_EDITOR

private:
   void ValidateText(FName logCategory) const;
};
