// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueFact.h"
#include "Variation/Clues/TATClueSpawner.h"

// ue
#include "GameplayTagContainer.h"
#include "StructUtils/InstancedStruct.h"

#include "TATNPCClueSpawnerComponent.generated.h"

/// Struct used to replicate clue data to clients
USTRUCT()
struct TAT_API FTATNPCDialogueClueData
{
   GENERATED_BODY()

   // Prompt displayed when interacting with an NPC to receive this clue.
   UPROPERTY()
   FText InteractionPrompt;

   // The clue message to present to the player upon first interaction
   UPROPERTY()
   FText ClueDialogue;

   // The clue message to present to the player upon subsequent interactions
   UPROPERTY()
   FText FollowUpDialogue;

   UPROPERTY()
   FGameplayTag SourceTag;

   FTATSharedClueFactThunk Facts;

   // FTATClueEffect s applied to player on receiving clue
   UPROPERTY(NotReplicated)
   TArray<FInstancedStruct> Effects;

   FORCEINLINE bool IsValid() const { return !ClueDialogue.IsEmpty(); }
};

/// Component for use on NPC spawners to configure a dialogue interaction which provides
/// a single clue to the player. Passes this data to UTATNPCClueComponent which handles
/// the actual player interaction.
UCLASS(meta = (BlueprintSpawnableComponent))
class TAT_API UTATNPCClueSpawnerComponent : public UTATClueSpawnerComponent
{
   GENERATED_BODY()

public:
   UTATNPCClueSpawnerComponent();

   // From UTATSpawnerComponent
   virtual FTATClueBucketKey GetClueBucket() const override;

   void AuthorityCacheClue(FTATNPCDialogueClueData&& newClue);

   FORCEINLINE const FTATNPCDialogueClueData& AuthorityGetClue() const { check(GetOwner()->HasAuthority()); return _clue; }
   FORCEINLINE bool AuthorityHasClueToShare() const { return AuthorityGetClue().IsValid(); }

   DECLARE_DELEGATE(FOnClueEventNative);
   FOnClueEventNative OnSpawnerClueCachedNative;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnClueEvent);
   UPROPERTY(BlueprintAssignable, meta = (DeprecatedProperty))
   FOnClueEvent OnSpawnerClueCached;

public:
   // A tag that is matched with a placement tag on dialogue clues
   //
   // For example, if categorizing by rarity, a dialogue clue marked CluePlacement.Common would only go to a
   // dialogue clue spawner configured with CluePlacement.Common.
   UPROPERTY(EditAnywhere, meta=(Categories="CluePlacement"))
   FGameplayTag PlacementTag;

private:
   // Cache of the clue generated for this spawner. Will be retrieved by TATNPCClueComponent.
   UPROPERTY(Transient)
   FTATNPCDialogueClueData _clue;

};
