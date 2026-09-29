// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSEIndividualAttitudeComponent.h"
#include "Perception/TATResettableAIKnowledgeContainer.h"
#include "TATIndividualAttitudeComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATIndividualAttitudeComponent : public UOSEIndividualAttitudeComponent,
                                                public ITATResettableAIKnowledgeContainer
{
   GENERATED_BODY()

public:
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   
   // begin ITATResettableAIKnowledgeContainer
   virtual void ResetKnowledgeOfActor(AActor* actor) override;
   // end ITATResettableAIKnowledgeContainer
};
