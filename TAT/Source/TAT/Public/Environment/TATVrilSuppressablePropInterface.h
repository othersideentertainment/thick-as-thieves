// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATVrilSuppressablePropInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UTATVrilSuppressablePropInterface : public UInterface
{
	GENERATED_BODY()
};


// A prop that can be temporarily disrupted by vril
class TAT_API ITATVrilSuppressablePropInterface
{
	GENERATED_BODY()

	// Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
   UFUNCTION(BlueprintCallable, Category = "Suppressable Prop")
   virtual void StartSuppressing(const AActor* source) = 0;

   UFUNCTION(BlueprintCallable, Category = "Suppressable Prop")
   virtual void StopSuppressing(const AActor* source) = 0;
};
