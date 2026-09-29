// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

#include "ItemFunctionLibrary.generated.h"

UCLASS()
class OSECORE_API UOSEItemFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, Category = "OSE|Tools")
   static void StowCurrentToolForAvatar(const AActor* actor, bool stowed);

   UFUNCTION(BlueprintPure, Category = "OSE|Tools")
   static bool CanAvatarAttackWithCurrentTool(const AActor* actor);
};
