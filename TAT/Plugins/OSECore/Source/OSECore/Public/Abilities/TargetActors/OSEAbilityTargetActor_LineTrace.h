// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Abilities/TargetActors/OSEAbilityTargetActor_BaseTrace.h"
#include "OSEAbilityTargetActor_LineTrace.generated.h"

/**
 * Override of engine GameplayAbilityTargetActor_SingleLineTrace to use our targeting functions
 * StartLocation does not need to be set explicitly, if set to default will use source actor's viewpoint
 * bTraceAffectsAimPitch is ignored as this only does one trace
 */
UCLASS()
class OSECORE_API AOSEAbilityTargetActor_LineTrace : public AOSEAbilityTargetActor_BaseTrace
{
   GENERATED_BODY()

public:
   AOSEAbilityTargetActor_LineTrace();

protected:
   virtual FHitResult PerformTrace(AActor* inSourceActor) override;
   virtual void ConfirmTargetingAndContinue() override;

   virtual bool _AllowHitAsTarget(const FHitResult& hit) { return true; }

   // pre-quantize the hit
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Trace)
   bool bPreQuantizeHitResult;
};
