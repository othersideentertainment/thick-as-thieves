// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATHitReactAnimInterface.generated.h"

UENUM(BlueprintType)
enum class ETATHitReactAnimDirection : uint8
{
   None,
   FrontLeft,
   FrontRight,
   BackLeft,
   BackRight,
};

UINTERFACE(BlueprintType, Category = "Hit React Anim Interface", meta = (CannotImplementInterfaceInBlueprint))
class TAT_API UTATHitReactAnimInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATHitReactAnimInterface
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintCallable, Category = "Hit React Anim Interface")
   virtual ETATHitReactAnimDirection GetHitReactAnimDirection() const = 0;

   UFUNCTION(BlueprintCallable, Category = "Hit React Anim Interface")
   virtual void SetHitReactAnimDirection(ETATHitReactAnimDirection direction) = 0;
};
