// (c) 2023-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Tools/OSECommandletBase.h"

#include "OSEUnusedTagsCommandlet.generated.h"

UCLASS(CustomConstructor, Config = Game)
class UOSEUnusedTagsCommandlet : public UOSECommandletBase
{
   GENERATED_UCLASS_BODY()

public:
   UOSEUnusedTagsCommandlet(const FObjectInitializer& objectInitializer);

protected:
   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("OSEUnusedTagsCommandlet"); };

private:
   void _FindUnusedTags(int32 referenceCount);
};
