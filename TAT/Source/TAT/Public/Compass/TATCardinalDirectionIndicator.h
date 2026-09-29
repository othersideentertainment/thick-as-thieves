// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"

// tat
#include "Compass/TATGenericIndicator.h"

#include "TATCardinalDirectionIndicator.generated.h"

UCLASS()
class TAT_API ATATCardinalDirectionIndicator : public ATATGenericIndicator
{
   GENERATED_BODY()
public:
   ATATCardinalDirectionIndicator();

   virtual FVector GetIndicatorLocation() const override;

   void SetCardinality(const FVector relativePlayerOffset, const FText displayText);

protected:
   // How far away from the player should this indicator report itself to be?
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float _offsetDistance = 10000.f;

private:
   // What direction (relative to the player) should this indicator show itself to be?
   FVector _relativePlayerOffset;
};
