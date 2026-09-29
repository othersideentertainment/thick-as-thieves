// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StructUtils/InstancedStruct.h"
#include "Engine/DataAsset.h"

#include "TATInventoryToken.generated.h"

class UPaperSprite;

// Static data for a inventory token
//
// Tokens are simple non-droppable things that have side effects when the character holds them
UCLASS(BlueprintType)
class TAT_API UTATInventoryToken : public UDataAsset
{
   GENERATED_BODY()

public:

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FText DisplayName;

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   TSoftObjectPtr<UPaperSprite> Icon;

   // Should this be singular?
   UPROPERTY(EditAnywhere, Meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATTokenEffect"))
   FInstancedStruct Effect;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};
