// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "TATPowerNetworkInterface.generated.h"

class UTATPowerNetworkComponent;
class USplineComponent;

// This class does not need to be modified.
UINTERFACE(BlueprintType, MinimalAPI, Category = "TAT|Electrical", meta=(CannotImplementInterfaceInBlueprint))
class UTATPowerNetworkInterface : public UInterface
{
   GENERATED_BODY()
};

/// Represents a part of the power network
class TAT_API ITATPowerNetworkInterface
{
   GENERATED_BODY()

public:
   /// Gets the power network component for this actor
   virtual UTATPowerNetworkComponent* GetPowerNetworkComponent() const = 0;

   /// Gets a spline that represents the "power line" for connectors
   virtual USplineComponent* GetPowerNetworkConnectorSplineComponent() const { return nullptr; }

   /// Gets a world location for a link point on a power network actor.
   /// Different power network actors can interpret the index differently - for example, a power line might have exactly two indices,
   /// where each represents one end of the power line.
   virtual TOptional<FVector> GetPowerNetworkWorldLocationForConnectionIndex(int32 index, bool forDebugVis) const { return NullOpt; }

   /// For power network providers (eg. power sources), check if they're currently powered
   virtual TOptional<bool> IsPowerNetworkProviderPowered() const { return NullOpt; }
};
