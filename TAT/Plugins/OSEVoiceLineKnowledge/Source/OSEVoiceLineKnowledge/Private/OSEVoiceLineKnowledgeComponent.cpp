// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "OSEVoiceLineKnowledgeComponent.h"

// ose
#include "OSEIndividualKnowledgeInterface.h"
#include "OSEVoiceLineTraitInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceLineKnowledgeComponent)

UOSEVoiceLineKnowledgeComponent::UOSEVoiceLineKnowledgeComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   bWantsInitializeComponent = true;
}

void UOSEVoiceLineKnowledgeComponent::InitializeComponent()
{
   Super::InitializeComponent();
   if(const IOSEIndividualKnowledgeInterface* individualKnowledgeInterface = Cast<IOSEIndividualKnowledgeInterface>(GetOwner()))
   {
      _individualKnowledgeComponent = individualKnowledgeInterface->GetIndividualKnowledgeComponent();
   }
}

const TObjectPtr<UOSEConditionalVoiceLineDataAsset>* UOSEVoiceLineKnowledgeComponent::GetVoiceLineDataAssetForTag(
   const FGameplayTag conditionalVoiceLineTag) const
{
   const TObjectPtr<UOSEConditionalVoiceLineDataAsset>* voiceLineDataPointer = _voiceLineDataMap.FindByPredicate(
   [conditionalVoiceLineTag](const TObjectPtr<UOSEConditionalVoiceLineDataAsset> line)
   {
      return line->IsDataRelevantToConditionalVoiceLineTag(conditionalVoiceLineTag);
   });   
   return voiceLineDataPointer;
}

bool UOSEVoiceLineKnowledgeComponent::GetVoiceVerbForConditionalVoiceLine(const AActor* actor, const FGameplayTag conditionalVoiceLineTag, FGameplayTag& outVoiceVerbTag) const
{
   FGameplayTagContainer containerToUseForMatching;
   if(_individualKnowledgeComponent != nullptr)
   {
      if(const FIndividualKnowledge* knowledge = _individualKnowledgeComponent->FindKnowledgeForActor(actor))
      {
         // Creating a copy right now, maybe we can cache off the traits on actor knowledge creation instead of at this
         // stage? This version allows for dynamic trait additions though..
        containerToUseForMatching.AppendTags(knowledge->GameplayTagContainer);
      }
   }
   
   if(const IOSEVoiceLineTraitInterface* lineTraitInterface = Cast<IOSEVoiceLineTraitInterface>(actor))
   {
      lineTraitInterface->GetActorTraitsForVoiceLines(containerToUseForMatching);
   }

   return GetVoiceVerbForConditionalVoiceLineAndContainer(conditionalVoiceLineTag, containerToUseForMatching, outVoiceVerbTag);
}

bool UOSEVoiceLineKnowledgeComponent::GetVoiceVerbForConditionalVoiceLineAndContainer(
   const FGameplayTag conditionalVoiceLineTag,
   const FGameplayTagContainer& voiceLineTagContainer,
   FGameplayTag& outVoiceVerbTag) const
{
   const TObjectPtr<UOSEConditionalVoiceLineDataAsset>* voiceLineDataPtr = GetVoiceLineDataAssetForTag(conditionalVoiceLineTag);
   if(voiceLineDataPtr == nullptr)
      return false;
   const TObjectPtr<UOSEConditionalVoiceLineDataAsset> voiceLineData = *voiceLineDataPtr;
   if(voiceLineData == nullptr)
      return false;
   outVoiceVerbTag = voiceLineData->FindLineForTags(voiceLineTagContainer);
   return true;
}
