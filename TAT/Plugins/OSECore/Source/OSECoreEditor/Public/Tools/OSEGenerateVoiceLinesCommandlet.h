// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

//ose
#include "OSECommandletBase.h"

//ue4
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "OSEGenerateVoiceLinesCommandlet.generated.h"

class UOSEVoiceOverLine;
class IFileHandle;

UCLASS(CustomConstructor)
class UOSEGenerateVoiceLinesCommandlet : public UOSECommandletBase
{
   GENERATED_BODY()

public:
   UOSEGenerateVoiceLinesCommandlet(const FObjectInitializer& objectInitializer);

protected:
   // from UOSECommandletBase
   virtual int _RunOSECommandlet(const FString& fullCommandLine) override;
   virtual const TCHAR* _GetOSECommandletName() const override { return TEXT("OSEGenerateVoiceLines"); };

};
