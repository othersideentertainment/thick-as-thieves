// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "UObject/Interface.h"

#include "TATNavModifierChildrenDataInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(MinimalAPI)
class UTATNavModifierChildrenDataInterface : public UInterface
{
	GENERATED_BODY()
};

class TAT_API ITATNavModifierChildrenDataInterface
{
	GENERATED_BODY()

public:
   virtual void GetStaticNavModifierOffsetTransform(FTransform& offset) const { return offset.SetIdentity(); }
};
