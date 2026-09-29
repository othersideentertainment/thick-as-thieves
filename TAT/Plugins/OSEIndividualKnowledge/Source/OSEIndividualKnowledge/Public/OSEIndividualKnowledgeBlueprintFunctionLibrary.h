// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTags.h"

// ose 
#include "OSEIndividualKnowledgeComponent.h"

#include "OSEIndividualKnowledgeBlueprintFunctionLibrary.generated.h"

UCLASS()
class OSEINDIVIDUALKNOWLEDGE_API UOSEIndividualKnowledgeBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()
public:
   UFUNCTION(BlueprintCallable, BlueprintPure)
   static UOSEIndividualKnowledgeComponent* GetIndividualKnowledgeComponent(AActor* actor);
   
   UFUNCTION(BlueprintCallable)
   static bool SetIndividualKnowledge(AActor* actor, AActor* target, FGameplayTag tag);

   UFUNCTION(BlueprintCallable)
   static bool HasIndividualKnowledge(AActor* actor, AActor* target, FGameplayTag tag);
   
   UFUNCTION(BlueprintCallable)
   static bool UnSetIndividualKnowledgeForActor(AActor* actor, AActor* target, FGameplayTag tag);

   UFUNCTION(BlueprintCallable)
   static void ClearIndividualKnowledgeOfActor(AActor* actor, const AActor* target);
};
