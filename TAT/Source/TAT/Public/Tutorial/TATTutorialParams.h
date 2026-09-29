// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATTutorialParams.generated.h"

class ATATPlayerState;

USTRUCT(BlueprintType)
struct FTATTutorialParams
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadOnly)
   TObjectPtr<UWorld> World = nullptr;

   UPROPERTY(BlueprintReadOnly)
   TObjectPtr<ATATPlayerState> PlayerState = nullptr;

   UPROPERTY(BlueprintReadOnly)
   TObjectPtr<ACharacter> Character = nullptr;

   UPROPERTY(BlueprintReadOnly)
   TObjectPtr<APlayerController> Controller = nullptr;
};
