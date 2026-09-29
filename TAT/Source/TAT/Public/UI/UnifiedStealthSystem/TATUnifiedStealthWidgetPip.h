// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "TATUnifiedStealthWidgetPip.generated.h"

UCLASS()
class TAT_API UTATUnifiedStealthWidgetPip : public UUserWidget
{
   GENERATED_BODY()

public:
   void SetState(bool state);
protected:
   UFUNCTION(BlueprintNativeEvent)
   void _HandleStateChange(bool isActivated);
private:
   TOptional<bool> _IsCurrentlyActive;
};
