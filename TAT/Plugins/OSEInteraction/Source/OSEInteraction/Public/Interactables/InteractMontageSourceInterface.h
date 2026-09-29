// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "InteractMontageSourceInterface.generated.h"

class UInteractMontageMappingAsset;

// This class does not need to be modified.
UINTERFACE(BlueprintType, MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UInteractMontageSourceInterface : public UInterface
{
   GENERATED_BODY()
};

/// Interface to be implemented by characters that have interact montages to be played
class OSEINTERACTION_API IInteractMontageSourceInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:
   // Gets the interact montage mapping for this object
   UFUNCTION(BlueprintCallable, Category = "Interactable|Animation")
   virtual UInteractMontageMappingAsset* GetInteractMontageMapping() const = 0;
};
