// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTags.h"
#include "Engine/DataAsset.h"

#include "OSEConditionalVoiceLineDataAsset.generated.h"

USTRUCT()
struct FVoiceLineWithWeight
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere)
   FGameplayTag TagToPlay;

   UPROPERTY(EditAnywhere, meta=(UIMin=1))
   int Weight { 1 };
};

USTRUCT()
struct FVoiceLineWithCondition
{
   GENERATED_BODY()
   
public:
   bool Matches(const FGameplayTagContainer& container) const;
   const FGameplayTag& GetTagToPlay() const;
   void SetupWeightsAndComplexity();

   int GetCachedQueryComplexity() const { return _cachedQueryComplexity; };

private:
   int _CalculateQueryComplexity() const;

#if WITH_EDITORONLY_DATA
   UPROPERTY(EditDefaultsOnly)
   FName DebugName;
#endif
   
   UPROPERTY(EditAnywhere, DisplayName="Tag to fall back to if weights aren't set")
   FGameplayTag _tagToPlayIfConditionsMet;

   UPROPERTY(EditAnywhere, DisplayName="Tags with Weights", meta = (TitleProperty = "{TagToPlay} ({Weight})"))
   TArray<FVoiceLineWithWeight> _tagsToPlayIfConditionsMetWithWeights;

   UPROPERTY(EditAnywhere)
   FGameplayTagQuery _tagConditionsToMeet;

   UPROPERTY(VisibleDefaultsOnly)
   int _totalWeight { 0 };
   UPROPERTY(VisibleDefaultsOnly)
   int _cachedQueryComplexity { 0};
};

UCLASS()
class OSEVOICELINEKNOWLEDGE_API UOSEConditionalVoiceLineDataAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   bool IsDataRelevantToConditionalVoiceLineTag(const FGameplayTag& gameplayTag) const;
   const FGameplayTag& FindLineForTags(const FGameplayTagContainer& container) const;

#if WITH_EDITOR
   virtual void PreSave(FObjectPreSaveContext saveContext) override;
#endif
private:
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag _voiceLineTag;
   
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag _defaultVoiceVerbTag;

   UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "{DebugName}"))
   TArray<FVoiceLineWithCondition> _linesWithConditions;
};
