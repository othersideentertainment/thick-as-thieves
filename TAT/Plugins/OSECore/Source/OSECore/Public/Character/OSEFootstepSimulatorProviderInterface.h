// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue4
#include "CoreMinimal.h"

#include "OSEFootstepSimulatorProviderInterface.generated.h"


class UOSEFootstepSimulatorComponent;

UINTERFACE(MinimalAPI, Category = "Character|OSE", meta = (CannotImplementInterfaceInBlueprint))
class UOSEFootstepSimulatorProviderInterface : public UInterface
{
   GENERATED_BODY()
};

class OSECORE_API IOSEFootstepSimulatorProviderInterface
{
   GENERATED_BODY()
public:
   UFUNCTION(BlueprintCallable, Category = "Character|OSE")
   virtual UOSEFootstepSimulatorComponent* GetFootstepComponent() const = 0;
};
