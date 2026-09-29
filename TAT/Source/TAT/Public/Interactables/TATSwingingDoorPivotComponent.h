// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/Navigation/TATNavModifierChildrenDataInterface.h"

// ue
#include "Components/StaticMeshComponent.h"

#include "TATSwingingDoorPivotComponent.generated.h"

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class TAT_API UTATSwingingDoorPivotComponent : public UStaticMeshComponent,
   public ITATNavModifierChildrenDataInterface
{
	GENERATED_BODY()
	
public:
   // from ITATNavModifierChildrenDataInterface
   virtual void GetStaticNavModifierOffsetTransform(FTransform& offset) const override;

   void SetOffsetFromClosed(const FTransform& offset);

private:
   FTransform _closedOffset = FTransform::Identity;
};
