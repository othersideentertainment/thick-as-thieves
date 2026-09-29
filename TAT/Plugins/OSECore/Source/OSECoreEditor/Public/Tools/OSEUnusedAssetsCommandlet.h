// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Tools/OSECommandletBase.h"

#include "OSEUnusedAssetsCommandlet.generated.h"

UCLASS(CustomConstructor, Config = Game)
class UOSEUnusedAssetsCommandlet : public UOSECommandletBase
{
   GENERATED_UCLASS_BODY()

public:
   UOSEUnusedAssetsCommandlet(const FObjectInitializer& objectInitializer);

protected:
   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("OSEUnusedAssetsCommandlet"); };

private:
   void _FindUnusedAssets();
};
