// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE
#include <Input/CommonUIActionRouterBase.h>

#include "TATActionRouter.generated.h"

UCLASS(MinimalAPI)
class UTATActionRouter : public UCommonUIActionRouterBase
{
   GENERATED_BODY()

protected:
   virtual TSharedRef<FCommonAnalogCursor> MakeAnalogCursor() const override;
};
