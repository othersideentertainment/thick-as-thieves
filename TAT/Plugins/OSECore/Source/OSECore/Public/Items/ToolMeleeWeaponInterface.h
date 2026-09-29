// (c) 2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "ToolMeleeWeaponInterface.generated.h"



UINTERFACE(BlueprintType, MinimalAPI, Category = "Tools")
class UToolMeleeWeaponInterface : public UInterface
{
   GENERATED_BODY()
};


class OSECORE_API IToolMeleeWeaponInterface
{
   GENERATED_BODY()
public:

   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat|OSE")
   void OnBlock(const FVector& location, const FVector& normal);

   UFUNCTION(BlueprintCallable, BlueprintImplementableEvent, Category = "Combat|OSE")
   void OnBlockBroke(const FVector& location, const FVector& normal);
};
