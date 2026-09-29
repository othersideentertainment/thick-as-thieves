// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameplayTagContainer.h"
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TATClueSourceInterface.generated.h"


// This class does not need to be modified.
UINTERFACE(meta = (CannotImplementInterfaceInBlueprint))
class UTATClueSourceInterface : public UInterface
{
   GENERATED_BODY()
};

// An interface to allow spawned actor classes to advertise that they have clues
// Immediate use-case is major loot producing clues
//
// Only expected to be called on the CDO
//
// Initially tries to use the asset registry, but that ran into issues with the tags from the CDO getting added to the
// Blueprint and not the BlueprintGeneratedClass.
//
// CONSIDER: Does this name sound broader than it is?
class TAT_API ITATClueSourceInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   struct FClueSourceParams
   {
      FGameplayTag SourceTag;
      FSoftObjectPath FallbackClueSet;
   };

   virtual TOptional<FClueSourceParams> GetClueParameters() const = 0;
};
