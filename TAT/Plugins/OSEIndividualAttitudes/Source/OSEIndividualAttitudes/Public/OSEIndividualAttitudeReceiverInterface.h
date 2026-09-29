// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OSEIndividualAttitudeReceiverInterface.generated.h"

enum class EOSEIndividualAttitude : uint8;

// This class does not need to be modified.
UINTERFACE()
class OSEINDIVIDUALATTITUDES_API UOSEIndividualAttitudeReceiverInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEINDIVIDUALATTITUDES_API IOSEIndividualAttitudeReceiverInterface
{
   GENERATED_BODY()

public:
   virtual void AttitudeChangedFromActor(AActor* sourceOfChange, EOSEIndividualAttitude attitude) = 0;
   virtual EOSEIndividualAttitude GetAttitudeFromActor(const AActor* sourceOfChange) const = 0;
};
