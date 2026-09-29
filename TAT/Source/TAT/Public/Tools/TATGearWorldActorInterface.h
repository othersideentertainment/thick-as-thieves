// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "UObject/Interface.h"

#include "TATGearWorldActorInterface.generated.h"

struct FTATGearWorldActorParameters;

UINTERFACE(BlueprintType, MinimalAPI, Category = "Tools")
class UTATGearWorldActorInterface : public UInterface
{
   GENERATED_BODY()
};

/// Implemented by world actors that are meant to be spawned by tools. Not required for them,
/// but useful if they want access to the specified parameters
/// Currently also used by some projectiles as an intermediate storage for the information
class TAT_API ITATGearWorldActorInterface
{
   GENERATED_BODY()

public:

   /// Called on the world actor when it is first created. The parameters allow for customization
   /// by the spawned actor
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, BlueprintNativeEvent, Category = "TAT|Tools")
   void AuthorityDeploy(const FTATGearWorldActorParameters& worldActorParams);
   virtual void AuthorityDeploy_Implementation(const FTATGearWorldActorParameters& worldActorParams) { }
};


