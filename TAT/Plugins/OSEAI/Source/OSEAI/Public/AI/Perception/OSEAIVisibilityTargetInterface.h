// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OSEAIVisibilityTargetInterface.generated.h"


UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UOSEAIVisibilityTargetInterface : public UInterface
{
   GENERATED_BODY()
};


/// <summary>
/// Use this interface to let a character know that it is visible to an Actor.
/// </summary>
class OSEAI_API IOSEAIVisibilityTargetInterface
{
   GENERATED_BODY()

   // Add interface functions to this class. This is the class that will be inherited to implement this interface.
public:

   virtual void AuthorityOnEnterVisibleByActor(AActor* viewingActor) = 0;
   virtual void AuthorityOnExitVisibleByActor(AActor* viewingActor) = 0;
};
