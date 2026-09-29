// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "OSECommandletBase.h"
#include "UObject/Interface.h"
#include "OSEDataValidationCommandlet.generated.h"

UCLASS(CustomConstructor)
class UOSEDataValidationCommandlet : public UOSECommandletBase
{
   GENERATED_UCLASS_BODY()

public:
   UOSEDataValidationCommandlet(const FObjectInitializer& objectInitializer);

   // do the validation without creating a commandlet
   static bool ValidateData(TArrayView<FString> ignoreFolderRoots = {});

protected:
   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("OSEDataValidationCommandlet"); };
};
