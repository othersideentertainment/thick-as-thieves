// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "TATLockConfig.h"
#include "UObject/Interface.h"
#include "TATLockableInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(NotBlueprintable)
class UTATLockableInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATLockableInterface
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable)
   virtual FLockInteractContext MakeLockContext(ACharacter* interactingCharacter) const = 0;
};
