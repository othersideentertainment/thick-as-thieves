// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Perception/OSEAIPerceptionComponent.h"

// ue4
#include "CoreMinimal.h"

#include "TATAIPerceptionComponent.generated.h"

class AOSEAIController;
class UAISenseConfig_Sight;

UCLASS(meta=(BlueprintSpawnableComponent))
class TAT_API UTATAIPerceptionComponent : public UOSEAIPerceptionComponent
{
   GENERATED_BODY()

public:
   UPROPERTY(BlueprintAssignable)
   FActorPerceptionUpdatedDelegate OnTATHearingEvent;

#if WITH_GAMEPLAY_DEBUGGER
   virtual void DescribeSelfToGameplayDebugger(FGameplayDebuggerCategory* debuggerCategory) const override;
#endif // WITH_GAMEPLAY_DEBUGGER

protected:
   virtual void _ShareKnowledgeWithActorFromSight(const AActor* actor) const;
   virtual void _OnTargetPerceptionUpdated(AActor* actor, FAIStimulus stimulus) override;
   virtual void _OnTargetPerceptionInfoUpdated(const FActorPerceptionUpdateInfo& updateInfo) override;
};
