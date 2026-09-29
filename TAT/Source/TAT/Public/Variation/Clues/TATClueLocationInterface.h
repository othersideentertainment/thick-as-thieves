// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TATClueLocationInterface.generated.h"

struct FGameplayTag;

// This class does not need to be modified.
UINTERFACE(meta=(CannotImplementInterfaceInBlueprint))
class UTATClueLocationInterface : public UInterface
{
   GENERATED_BODY()
};

// An interface that represent the location that a clue refers to
// (not the location where the clue appears)
//
// e.g. The shiny hat is in the lord's bedroom
class TAT_API ITATClueLocationInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   // May refactor into some sort of append-to-bag-of-properties
   virtual const FText& GetClueLocationName() const = 0;

   // Just for identity purposes
   virtual const FGameplayTag& GetClueLocationTag() const = 0;
};

UCLASS()
class TAT_API UTATDummyClueLocation : public UObject, public ITATClueLocationInterface
{
   GENERATED_BODY()

public:
   static UTATDummyClueLocation* Get() { return GetMutableDefault<UTATDummyClueLocation>(); }
   
   virtual const FText& GetClueLocationName() const override;
   virtual const FGameplayTag& GetClueLocationTag() const override;
};
