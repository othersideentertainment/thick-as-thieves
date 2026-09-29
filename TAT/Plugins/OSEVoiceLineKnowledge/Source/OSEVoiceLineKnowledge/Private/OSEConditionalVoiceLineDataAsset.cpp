// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "OSEConditionalVoiceLineDataAsset.h"

// ue
#include "UObject/ObjectSaveContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEConditionalVoiceLineDataAsset)

bool FVoiceLineWithCondition::Matches(const FGameplayTagContainer& container) const
{
   return container.MatchesQuery(_tagConditionsToMeet);
}

const FGameplayTag& FVoiceLineWithCondition::GetTagToPlay() const
{
   int randomValue = FMath::RandRange(0, _totalWeight);
   for (const FVoiceLineWithWeight& element : _tagsToPlayIfConditionsMetWithWeights)
   {
      if(randomValue < element.Weight)
      {
         return element.TagToPlay;
      }
      randomValue -= element.Weight;
   }
   // we shouldn't hit here, but if we do, return the default.
   return _tagToPlayIfConditionsMet;
}

void FVoiceLineWithCondition::SetupWeightsAndComplexity()
{
   _cachedQueryComplexity = _CalculateQueryComplexity();
   _totalWeight = 0;
   for (FVoiceLineWithWeight& element : _tagsToPlayIfConditionsMetWithWeights)
   {
      _totalWeight += element.Weight;
   }
}

int FVoiceLineWithCondition::_CalculateQueryComplexity() const
{
   TArray<FGameplayTag> outTags;
   _tagConditionsToMeet.GetGameplayTagArray(outTags);
   return outTags.Num();
}

bool UOSEConditionalVoiceLineDataAsset::IsDataRelevantToConditionalVoiceLineTag(const FGameplayTag& gameplayTag) const
{
   return gameplayTag.MatchesTag(_voiceLineTag);
}

const FGameplayTag& UOSEConditionalVoiceLineDataAsset::FindLineForTags(const FGameplayTagContainer& container) const
{
   int indexOfBestMatchingTag = INDEX_NONE;
   int queryComplexityOfBestMatchingTag = INDEX_NONE;
   for (int i=0; i < _linesWithConditions.Num(); ++i)
   {
      const FVoiceLineWithCondition& voiceLineWithCondition = _linesWithConditions[i];
      const int cachedQueryComplexity = voiceLineWithCondition.GetCachedQueryComplexity();
      if(cachedQueryComplexity > queryComplexityOfBestMatchingTag)
      {
         if(voiceLineWithCondition.Matches(container))
         {
            indexOfBestMatchingTag = i;
            queryComplexityOfBestMatchingTag = cachedQueryComplexity;
         }
      }
   }

   if(_linesWithConditions.IsValidIndex(indexOfBestMatchingTag) == false)
      return _defaultVoiceVerbTag;

   return _linesWithConditions[indexOfBestMatchingTag].GetTagToPlay();
}

#if WITH_EDITOR
void UOSEConditionalVoiceLineDataAsset::PreSave(FObjectPreSaveContext saveContext)
{
   for (FVoiceLineWithCondition& linesWithCondition : _linesWithConditions)
   {
      linesWithCondition.SetupWeightsAndComplexity();
   }
   Super::PreSave(saveContext);
}
#endif
