// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <CommonActivatableWidget.h>

#include "TATActivatableWidget.generated.h"

UENUM(BlueprintType)
enum class ETATActivatableWidgetInputMode : uint8
{
   Default,
   Menu,
   Game,
   All,
};

UCLASS(MinimalAPI, Abstract, meta=(DisableNativeTick))
class UTATActivatableWidget : public UCommonActivatableWidget
{
   GENERATED_BODY()

public:
   virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override final;

   // Only used by tutorial, if true swallows back action inpit
   bool SwallowBackAction = false;

protected:
   virtual bool NativeOnHandleBackAction() override;
   
   UPROPERTY(EditDefaultsOnly, Category = Input)
   ETATActivatableWidgetInputMode InputMode = ETATActivatableWidgetInputMode::Default;

   UPROPERTY(EditDefaultsOnly, Category = Input)
   EMouseCaptureMode GameMouseCaptureMode = EMouseCaptureMode::CapturePermanently;
};
