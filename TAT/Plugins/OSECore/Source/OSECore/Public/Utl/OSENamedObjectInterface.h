// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "UObject/Interface.h"

#include "OSENamedObjectInterface.generated.h"

UINTERFACE(BlueprintType, Category = "Named Object")
class UOSENamedObjectInterface : public UInterface
{
   GENERATED_BODY()
};

class OSECORE_API IOSENamedObjectInterface
{
   GENERATED_BODY()

public:

   // What is the name of this object?
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Named Object")
   FText GetObjectName() const;
};
