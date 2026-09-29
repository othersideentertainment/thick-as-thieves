// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "NavLinkCustomComponent.h"
#include "AI/Navigation/TATNavLinkCustomComponent.h"
#include "SwingingDoorNavLinkComponent.generated.h"

// A nav link component used by the swinging door
UCLASS()
class TAT_API USwingingDoorNavLinkComponent : public UTATNavLinkCustomComponent
{
   GENERATED_BODY()

   USwingingDoorNavLinkComponent();
   
public:
   virtual bool IsLinkPathfindingAllowed(const UObject* querier) const override;
   void SetLinkDirection(TEnumAsByte<ENavLinkDirection::Type> linkDirection) { LinkDirection = linkDirection; }

};
