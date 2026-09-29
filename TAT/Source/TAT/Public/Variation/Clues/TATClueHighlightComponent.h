// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/SceneVariants/TATSceneRequirement.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"

#include "TATClueHighlightComponent.generated.h"

// A helper component that notifies when the local player has learned a given clue fact
// (in order to trigger a highlight, or similar)
//
// CONSIDER: Is this name too specific?
// NOTE: No debug vis yet for scene requirement, as there is a decent chance it would be on an actor that has others
// NOTE: assumption that a single tag is sufficient will not last post-TOD, and uses of this will likely need to change
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATClueHighlightComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATClueHighlightComponent();

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClueFactLearned);
   UPROPERTY(BlueprintAssignable)
   FOnClueFactLearned OnClueFactLearned;

protected:
   virtual void BeginPlay() override;
   virtual bool NeedsLoadForServer() const override;

private:
   void _OnFactKnown();
   
   // Which fact triggers the highlight when learned
   UPROPERTY(EditAnywhere, Category=Clue, meta=(Categories="ClueFact"))
   FGameplayTag _clueFactTag;

   // The scene requirement that must be met before this is used
   UPROPERTY(EditAnywhere, Category=Clue)
   FTATSceneRequirement _highlightSceneRequirement;

};
