// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEVoiceLineKnowledgeBlueprintFunctionLibrary.h"

// ose
#include "OSEVoiceLineKnowledgeInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceLineKnowledgeBlueprintFunctionLibrary)

UOSEVoiceLineKnowledgeComponent* UOSEVoiceLineKnowledgeBlueprintFunctionLibrary::GetVoiceLineKnowledgeComponent(AActor* actor)
{
   const IOSEVoiceLineKnowledgeInterface* voiceLineKnowledgeInterface = Cast<IOSEVoiceLineKnowledgeInterface>(actor);
   if(voiceLineKnowledgeInterface == nullptr)
      return nullptr;   
   return voiceLineKnowledgeInterface->GetVoiceLineKnowledgeComponent();
}

FGameplayTag UOSEVoiceLineKnowledgeBlueprintFunctionLibrary::GetVoiceVerbFromKnowledge(AActor* actor,
   AActor* target,
   const FGameplayTag voiceLine)
{
   const UOSEVoiceLineKnowledgeComponent* voiceLineKnowledgeComponent = GetVoiceLineKnowledgeComponent(actor);
   if(voiceLineKnowledgeComponent == nullptr)
      return FGameplayTag::EmptyTag;
   FGameplayTag outVoiceVerb;
   if(voiceLineKnowledgeComponent->GetVoiceVerbForConditionalVoiceLine(target, voiceLine, outVoiceVerb))
      return outVoiceVerb;
   return FGameplayTag::EmptyTag;
}

FGameplayTag UOSEVoiceLineKnowledgeBlueprintFunctionLibrary::GetVoiceVerbFromConditionalLineWithTagContainer(
   AActor* actor,
   const FGameplayTag voiceLine,
   const FGameplayTagContainer& container)
{
   const UOSEVoiceLineKnowledgeComponent* voiceLineKnowledgeComponent = GetVoiceLineKnowledgeComponent(actor);
   if(voiceLineKnowledgeComponent == nullptr)
      return FGameplayTag::EmptyTag;
   FGameplayTag outVoiceVerb;
   if(voiceLineKnowledgeComponent->GetVoiceVerbForConditionalVoiceLineAndContainer(voiceLine, container, outVoiceVerb))
      return outVoiceVerb;
   return FGameplayTag::EmptyTag;
}
