// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/ActorComponent.h"

#include "TATFireflyPerceiverComponent.generated.h"


class UTATFireflyBeaconComponent;

// [DEPRECATED] A component that tracks which firefly beacons are shown
UCLASS(Blueprintable, ClassGroup=(Firefly), meta=(BlueprintSpawnableComponent))
class TAT_API UTATFireflyPerceiverComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATFireflyPerceiverComponent();

};
