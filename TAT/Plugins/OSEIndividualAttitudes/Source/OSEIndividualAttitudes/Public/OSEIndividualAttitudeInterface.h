// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "OSEIndividualAttitudeComponent.h"

// ue
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "OSEIndividualAttitudeInterface.generated.h"

UINTERFACE()
class OSEINDIVIDUALATTITUDES_API UOSEIndividualAttitudeInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEINDIVIDUALATTITUDES_API IOSEIndividualAttitudeInterface
{
   GENERATED_BODY()

public:
   virtual UOSEIndividualAttitudeComponent* GetAttitudeComponent() const= 0;
};
