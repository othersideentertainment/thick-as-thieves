// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Alertness/OSEAlertnessComponent.h"

#include "OSEAlertnessInterface.generated.h"

UINTERFACE(BlueprintType, MinimalAPI, Category = "AI|Alertness", meta = (CannotImplementInterfaceInBlueprint))
class UOSEAlertnessInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEAI_API IOSEAlertnessInterface
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintCallable, Category = "AI|Alertness")
   virtual UOSEAlertnessComponent* GetAlertnessComponent() const = 0;

   UFUNCTION(BlueprintCallable, Category = "AI|Alertness")
   virtual EAlertnessLevel GetAlertnessLevel() const = 0;
};
