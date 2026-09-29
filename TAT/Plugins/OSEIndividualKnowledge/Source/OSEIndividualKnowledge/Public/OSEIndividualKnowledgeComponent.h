// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"

#include "OSEIndividualKnowledgeComponent.generated.h"

USTRUCT()
struct OSEINDIVIDUALKNOWLEDGE_API FIndividualKnowledge
{
   GENERATED_BODY()
   FGameplayTagContainer GameplayTagContainer {};
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class OSEINDIVIDUALKNOWLEDGE_API UOSEIndividualKnowledgeComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UOSEIndividualKnowledgeComponent();

   void AddTag(const AActor* actor, const FGameplayTag tag, const float expirationTime = 0.f);
   bool HasTag(const AActor* actor, const FGameplayTag tag);
   void RemoveTag(const AActor* actor, const FGameplayTag tag);
   void ClearAllTags(const AActor* actor);
   const FIndividualKnowledge* FindKnowledgeForActor(const AActor* actor) const;

protected:
   FIndividualKnowledge& FindOrAddKnowledgeForActor(const AActor* actor);
   UFUNCTION()
   void OnRemoveAfterTime(const AActor* actor, FGameplayTag gameplayTag);
   
private:
   UPROPERTY()
   TMap<TWeakObjectPtr<const AActor>, FIndividualKnowledge> _knowledgeMap;
};
