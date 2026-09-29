// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

// tat
#include "TATSessionGameMode.h"

#include "TATPvEGameMode.generated.h"

UCLASS()
class TAT_API ATATPvEGameMode : public ATATSessionGameMode
{
   GENERATED_BODY()
public:
   virtual void PostLogin(APlayerController* newPlayer) override;
};
