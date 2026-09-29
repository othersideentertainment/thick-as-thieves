// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATClueSetBase.h"

// ue
#include "GameplayTagContainer.h"

#include "TATCompoundClueSet.generated.h"

struct FInstancedStruct;
class UTATMatchQuestDescription;

USTRUCT()
struct FTATClueCluster
{
   GENERATED_BODY()
   
   // Quest choice tags that must be matched for these clues to be used
   UPROPERTY(EditDefaultsOnly, meta=(Categories="QuestChoice"))
   FGameplayTagContainer RequiredTags;

   // Optional clue sets to use clues from if this cluster is used
   UPROPERTY(EditDefaultsOnly)
   TArray<TObjectPtr<UTATClueSetBase>> ClueSets;

   // Inline clues to use
   UPROPERTY(EditDefaultsOnly, meta = (ExcludeBaseStruct, BaseStruct = "/Script/TAT.TATClueInfo"))
   TArray<FInstancedStruct> Clues;
};

// A clue set that has multiple sections that can depend on
// different tag requirements
//
// Intended for use with match quests (OLD)
// TODO: Delete?
UCLASS()
class TAT_API UTATCompoundClueSet : public UTATClueSetBase
{
   GENERATED_BODY()

public:
   virtual void AddRelevantClueViews(const FTATClueSetContext& context, TArray<FConstStructView>& result) const override final;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(class FDataValidationContext& context) const override;
   virtual void VisitClueSetDependencies(TFunctionRef<void (const UTATClueSetBase*)> visitor) const override;
#endif

private:
#if WITH_EDITORONLY_DATA
   // OPTIONAL match quest description to validate tag requirements against
   // TODO: Try to filter available tags in input?
   UPROPERTY(EditDefaultsOnly, Category="Validation")
   TObjectPtr<UTATMatchQuestDescription> MatchQuestDescription = nullptr;
#endif

   // TODO-CLUES: Unit tests?
   
   UPROPERTY(EditDefaultsOnly, Category="Clues", meta=(TitleProperty="{RequiredTags}"))
   TArray<FTATClueCluster> _clueClusters;
};
