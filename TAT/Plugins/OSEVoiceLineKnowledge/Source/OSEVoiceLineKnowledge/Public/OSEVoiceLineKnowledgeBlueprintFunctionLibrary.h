// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"

// ose
#include "OSEVoiceLineKnowledgeComponent.h"

#include "OSEVoiceLineKnowledgeBlueprintFunctionLibrary.generated.h"

UCLASS()
class OSEVOICELINEKNOWLEDGE_API UOSEVoiceLineKnowledgeBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, BlueprintPure)
   static UOSEVoiceLineKnowledgeComponent* GetVoiceLineKnowledgeComponent(AActor* actor);

   UFUNCTION(BlueprintCallable)
   static FGameplayTag GetVoiceVerbFromKnowledge(AActor* actor, AActor* target, FGameplayTag voiceLine);
   
   UFUNCTION(BlueprintCallable)
   static FGameplayTag GetVoiceVerbFromConditionalLineWithTagContainer(AActor* actor,
                                                                       const FGameplayTag voiceLine,
                                                                       const FGameplayTagContainer& container);
};
