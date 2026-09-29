// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Tools/OSECommandletBase.h"
#include "UObject/Interface.h"
#include "OSEDuplicateWorldSettingsFixupCommandlet.generated.h"

UCLASS(CustomConstructor)
class UOSEDuplicateWorldSettingsFixupCommandlet : public UOSECommandletBase
{
   GENERATED_UCLASS_BODY()

public:
   UOSEDuplicateWorldSettingsFixupCommandlet(const FObjectInitializer& objectInitializer);

protected:
   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("OSEDuplicateWorldSettingsFixupCommandlet"); };

private:
   void _RunFixup();
};
