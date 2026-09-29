// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "TATBreakableAudioInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTATBreakableAudioInterface : public UInterface
{
   GENERATED_BODY()
};


// An optional interface for breakables to expose an AkComponent (if they have more than one, or don't want to do a search)
class TAT_API ITATBreakableAudioInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Breakable|Audio")
   UAkComponent* GetAkComponentForBreakable() const;
};
