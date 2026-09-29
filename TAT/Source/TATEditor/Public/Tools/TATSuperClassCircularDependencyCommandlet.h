// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Tools/OSECommandletBase.h"
#include "Engine/DeveloperSettings.h"

#include "TATSuperClassCircularDependencyCommandlet.generated.h"

UCLASS()
class UTATSuperClassCircularDependencyCommandlet : public UOSECommandletBase
{
   GENERATED_BODY()

protected:
   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("TATSuperClassCircularDependencyCommandlet"); };

private:
   /// Returns the number of assets with circular dependencies
   int32 _RunValidationOnAssets();

   bool _ShouldSkipAssetPath(const FString& assetPath) const;
};
