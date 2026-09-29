// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Tools/OSECommandletBase.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/Interface.h"
#include "OSEMapCheckCommandlet.generated.h"

UCLASS(CustomConstructor, Config = Game)
class UOSEMapCheckCommandlet : public UOSECommandletBase
{
   GENERATED_UCLASS_BODY()

public:
   UOSEMapCheckCommandlet(const FObjectInitializer& objectInitializer);

protected:
   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("OSEMapCheckCommandlet"); };

private:
   void _RunMapChecks();

   UPROPERTY(Config)
   TArray<FFilePath> MapChecksToSkip;
};
