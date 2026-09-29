// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Lockpicking/TATLockCombinationName.h"
#include "Variation/Clues/TATClueSpawnTypes.h"
#include "Variation/SceneVariants/TATSceneSet.h"

// ue
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "StructUtils/InstancedStruct.h"

#include "TATMatchQuestDescription.generated.h"


USTRUCT()
struct TAT_API FTATQuestChoiceOption
{
   GENERATED_BODY()

#if WITH_EDITORONLY_DATA
   UPROPERTY(EditDefaultsOnly)
   FString DebugName;
#endif

   // Tags that are output if this option is selected
   // TODO: Decide if need/want to split out a primary tag that can be used as an identity
   UPROPERTY(EditDefaultsOnly, meta = (Categories = "QuestChoice"))
   FGameplayTagContainer AddedChoiceTags;

   // This scene variant will be selected if the option is chosen
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UTATSceneVariantConfig> SceneVariant = nullptr;

   // Map of text replacement key to lock combination "name"
   //
   // Example:
   // If you have a combination lock configured with CombinationName = "Foo",
   // and set "MyCombination" = "Foo", then {MyCombination} will be replaced
   // with the combination used by that lock (or locks).
   UPROPERTY(EditDefaultsOnly)
   TMap<FString, FTATLockCombinationNameRef> LockCombinations;

   // Additional things that can produce text replacement
   UPROPERTY(EditDefaultsOnly, Category="TextReplacement", meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATQuestFormatParamSource"))
   TArray<FInstancedStruct> FormatParamSources;
};

USTRUCT()
struct TAT_API FTATQuestChoice
{
   GENERATED_BODY()

#if WITH_EDITORONLY_DATA
   UPROPERTY(EditDefaultsOnly)
   FString DebugName;
#endif

   // The number of options to choose for this choice
   UPROPERTY(EditAnywhere, meta = (UIMin = 0, ClampMin = 0))
   FInt32Interval NumToChoose = FInt32Interval(1,1);

   // The options to choose between
   UPROPERTY(EditDefaultsOnly, meta = (TitleProperty="{DebugName} ({SceneVariant})"))
   TArray<FTATQuestChoiceOption> Options;
};

USTRUCT()
struct TAT_API FTATQuestChoicePlan
{
   GENERATED_BODY()

   FORCEINLINE void Reset()
   {
      ChoiceTags.Reset();
      ForcedSceneVariants.Reset();
      FormatParams.Reset();
   }

   UPROPERTY()
   FGameplayTagContainer ChoiceTags;

   UPROPERTY()
   TArray<TObjectPtr<UTATSceneVariantConfig>> ForcedSceneVariants;
   
   FTATClueFormatParams FormatParams;
};

struct FTATQuestChoiceParams
{
   int32 Seed = 0;
   TConstArrayView<TObjectPtr<UTATSceneVariantConfig>> VariantOverrides;
   TFunctionRef<const UTATSceneAsset* (const UTATSceneVariantConfig*)> FindParentScene;
};

// Static data that controls how a match quest controls or overrides
// map variation
//
// NOTE: Does not currently imply that that this is 1:1 with a specific quest.
//       Multiple quests could reuse a single one, but may not do so much.
//       (unless that changes later)
// TODO: Name is meh?
// TODO: Delete?
UCLASS()
class TAT_API UTATMatchQuestDescription : public UDataAsset
{
   GENERATED_BODY()

public:
   void GenerateChoices(FTATQuestChoicePlan& outQuestChoicePlan, const FTATQuestChoiceParams& params) const;

   TOptional<float> GetEndgameDurationOverride() const { return _useEndgameDurationOverride ? TOptional<float>(_endgameDurationOverride) : NullOpt; }

#if WITH_EDITOR
   virtual void GetAssetRegistryTags(FAssetRegistryTagsContext context) const override;
   static TArray<TSoftObjectPtr<UTATMatchQuestDescription>> FindForMap(const TSoftObjectPtr<UWorld>& map);
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
   void CheckLockCombinations(const TSet<FName>& validLockCombinations, TFunctionRef<void (const FText&)> reportError) const;
   FGameplayTagContainer CollectChoiceTags() const;
#endif
   
#if WITH_EDITORONLY_DATA
   // The map that this match quest is for, so that it can be validated against
   // (filtered to levels with spawn data)
   UPROPERTY(EditDefaultsOnly, Category=Validation, meta=(RequiredAssetDataTags="LevelIsRandomized=True"))
   TSoftObjectPtr<UWorld> Map;
#endif
   
   // randomized choices that are selected in a match quest
   UPROPERTY(EditDefaultsOnly, Category=Choices, meta=(TitleProperty="{DebugName} {NumToChoose}"))
   TArray<FTATQuestChoice> Choices;
   
   UPROPERTY(EditDefaultsOnly, Category=Scenes)
   TMap<TObjectPtr<UTATSceneSetAsset>, FTATSceneSetOverride> SceneSetOverrides;

   // convenience for unconditionally forcing specific variants to be enabled without overriding the scene set (or using choices)
   UPROPERTY(EditDefaultsOnly, Category=Scenes)
   TArray<TObjectPtr<UTATSceneVariantConfig>> ForcedSceneVariants;

   // Map of text replacement key to lock combination "name"
   //
   // Example:
   // If you have a combination lock configured with CombinationName = "Foo",
   // and set "MyCombination" = "Foo", then {MyCombination} will be replaced
   // with the combination used by that lock (or locks).
   UPROPERTY(EditDefaultsOnly, Category = TextReplacement)
   TMap<FString, FTATLockCombinationNameRef> LockCombinations;

   UPROPERTY(EditDefaultsOnly, Category="TextReplacement", meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATQuestFormatParamSource"))
   TArray<FInstancedStruct> FormatParamSources;

private:
   // Overrides the duration of the endgame phase when the quest objective is complete
   UPROPERTY(EditDefaultsOnly, Category=Quest, meta=(UIMin=1, ClampMin=1, Units="Seconds", EditCondition="_useEndgameDurationOverride"))
   float _endgameDurationOverride = 300;

   UPROPERTY(EditDefaultsOnly, Category=Quest, meta=(InlineEditConditionToggle))
   bool _useEndgameDurationOverride = false;
};
