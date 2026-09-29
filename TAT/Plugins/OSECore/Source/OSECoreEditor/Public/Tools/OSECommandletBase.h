// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "Commandlets/Commandlet.h"
#include "UObject/Interface.h"

#include "OSECommandletBase.generated.h"

OSECOREEDITOR_API DECLARE_LOG_CATEGORY_EXTERN(LogOSECommandlet, Log, All);

UCLASS(CustomConstructor)
class OSECOREEDITOR_API UOSECommandletBase : public UCommandlet
{
   GENERATED_UCLASS_BODY()

public:
   UOSECommandletBase(const FObjectInitializer& objectInitializer);

   // Begin UCommandlet Interface
   virtual int32 Main(const FString& fullCommandLine) override;
   // End UCommandlet Interface

protected:
   // for our subclasses 
   virtual int _RunOSECommandlet(const FString& fullCommandLine) { unimplemented(); return 0; }
   virtual const TCHAR* _GetOSECommandletName() const { unimplemented(); return TEXT("OSECommandletBase"); }
};
