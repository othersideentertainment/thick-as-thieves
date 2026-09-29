// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "OSECommandletBase.h"

#include "OSEDumpSourceFileCommandlet.generated.h"

UCLASS(CustomConstructor, Config = Game)
class UOSEDumpSourceFileCommandlet : public UOSECommandletBase
{
   GENERATED_UCLASS_BODY()

public:
   UOSEDumpSourceFileCommandlet(const FObjectInitializer& objectInitializer);

protected:
   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("OSEDumpSourceFileCommandlet"); };

private:
   void _DumpSourceFiles();
};
