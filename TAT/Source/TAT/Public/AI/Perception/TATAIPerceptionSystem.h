// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Perception/AIPerceptionSystem.h"

#include "TATAIPerceptionSystem.generated.h"

struct FTATAINoiseEvent;
class ITATResettableAIKnowledgeContainer;

USTRUCT()
struct FTATRegisteredGlobalID
{
   GENERATED_BODY()

   
   int RegisteredID { INDEX_NONE };
};
UCLASS(ClassGroup=AI, Config=Game, DefaultConfig)
class TAT_API UTATAIPerceptionSystem : public UAIPerceptionSystem
{
   GENERATED_BODY()
      
public:
   UTATAIPerceptionSystem(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   static void RegisterResettableKnowledgeContainer(UWorld* world, ITATResettableAIKnowledgeContainer* container);
   static void UnregisterResettableKnowledgeContainer(UWorld* world, ITATResettableAIKnowledgeContainer* container);
   static void ResetKnowledgeOfActor(UWorld* world, AActor* actor);

   int32 GenerateGlobalIdForHearingStim(const FTATAINoiseEvent& eventToGenerateIDFor);
   void ResetGlobalIDForInstigator(AActor* actor);

private:
   int32 _nextHearingStimId = 0;

   TArray<TWeakInterfacePtr<ITATResettableAIKnowledgeContainer>> _RegisteredResettableKnowledgeContainer;

   TMap<TObjectKey<AActor>, TMap<FGameplayTag, int>> _RegisteredCharactersToGlobalTagsAndIDs;
};
