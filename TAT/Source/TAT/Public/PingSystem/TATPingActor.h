// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "PingSystem/OSEPingActor.h"

#include "TATPingActor.generated.h"

class ATATPlayerState;

UCLASS()
class TAT_API ATATPingActor : public AOSEPingActor
{
   GENERATED_BODY()

public:
   virtual void BeginPlay() override;
   virtual void BeginReplication() override;
   virtual bool IsNetRelevantFor(const AActor* realViewer, const AActor* viewTarget, const FVector& srcLocation) const override;

protected:
   bool _IsPingValidForPlayerState(const ATATPlayerState* playerState) const;

   UFUNCTION(BlueprintNativeEvent)
   void _TriggerPing();
};
