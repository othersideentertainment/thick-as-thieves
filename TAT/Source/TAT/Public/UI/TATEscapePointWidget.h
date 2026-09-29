// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "TATUserWidget.h"
#include "TATEscapePointWidget.generated.h"

UCLASS(Blueprintable, BlueprintType, meta = (DisableNativeTick))
class TAT_API UTATEscapePointWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   // Responsible for removing self from viewport, along with any accompanying visual flair
   UFUNCTION(BlueprintImplementableEvent)
   void HandleEscapeRouteRemoved();
};
