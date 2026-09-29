// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSEIndividualKnowledgeComponent.h"
#include "Perception/TATResettableAIKnowledgeContainer.h"
#include "TATIndividualKnowledgeComponent.generated.h"

// In TAT our TATKnowledgeComponent could potentially contain the functionality within this component
// Adding / removing tags against the "knowledge" of an actor. However the "OSEKnowledgeComponent" exists to share
// functionality. CURRENTLY OSEKnowledgeComponent doesn't do _anything_, so it might be that we just roll this functionality
// of this class and the other class into something shared.

// The tags added and removed to actor knowledge here will persist after an actor is "forgotten", which is desired.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATIndividualKnowledgeComponent : public UOSEIndividualKnowledgeComponent,
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
