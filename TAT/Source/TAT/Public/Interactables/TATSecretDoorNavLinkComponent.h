// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Navigation/TATNavLinkCustomComponent.h"

#include "TATSecretDoorNavLinkComponent.generated.h"

// A nav link component used by secret doors
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TAT_API UTATSecretDoorNavLinkComponent : public UTATNavLinkCustomComponent
{
   GENERATED_BODY()

   UTATSecretDoorNavLinkComponent();

public:
   // from INavLinkCustomInterface (via UNavLinkCustomComponent)
   virtual bool IsLinkPathfindingAllowed(const UObject* querier) const override;

   // This nan-link will only be used if a querier is within this distance.
   UPROPERTY(EditAnywhere, meta = (Units = cm))
   float UseIfWithinDistance = 500.0f;	
};
