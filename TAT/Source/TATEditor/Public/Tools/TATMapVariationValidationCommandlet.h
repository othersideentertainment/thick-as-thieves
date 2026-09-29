// (c) 2021-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Tools/OSECommandletBase.h"
#include "Engine/DeveloperSettings.h"

#include "TATMapVariationValidationCommandlet.generated.h"

UCLASS(CustomConstructor, Config = Game)
class UTATMapVariationValidationCommandlet : public UOSECommandletBase
{
   GENERATED_UCLASS_BODY()

public:
   UTATMapVariationValidationCommandlet(const FObjectInitializer& objectInitializer);

protected:
   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("TATMapVariationValidationCommandlet"); };

private:
   void _RunValidation(bool onlyCooked);
};
