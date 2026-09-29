// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATSwingingDoorPivotComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSwingingDoorPivotComponent)

void UTATSwingingDoorPivotComponent::GetStaticNavModifierOffsetTransform(FTransform& offset) const
{
   offset = _closedOffset;
}

void UTATSwingingDoorPivotComponent::SetOffsetFromClosed(const FTransform& offset)
{
   _closedOffset = offset;
}
