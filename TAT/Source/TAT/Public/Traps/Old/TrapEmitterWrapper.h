// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Traps/Old/TrapEmitterInterface.h"

// ue4
#include "GameFramework/Actor.h"

#include "TrapEmitterWrapper.generated.h"

// A simple actor that implements ITrapEmitterInterface, and delegates triggers to any components
// that implement ITrapEmitterInterface
UCLASS(Blueprintable)
class TAT_API ATrapEmitterWrapper_Old : public AActor, public ITrapEmitterInterface_Old
{
   GENERATED_BODY()
   
public:   
   // Sets default values for this actor's properties
   ATrapEmitterWrapper_Old();

   virtual void OnTriggered_Implementation() override;
};
