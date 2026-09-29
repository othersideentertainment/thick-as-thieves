// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/Clues/TATNPCClueSpawnerComponent.h"

// tat
#include "Interactables/TATInteractHighlightUtils.h"
#include "Variation/Clues/TATKnownCluesComponent.h"
#include "Quests/TATQuestDependentActorHelpers.h"

// ue
#include "Variation/Clues/Effects/TATClueEffectHelpers.h"

#include "GameFramework/Character.h"
#include "GameFramework/PlayerState.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATNPCClueSpawnerComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATNPCClueSpawnerComponent, Log, All);

UTATNPCClueSpawnerComponent::UTATNPCClueSpawnerComponent()
{
   SetIsReplicatedByDefault(false);
   _clueType = ETATClueType::NPC;
}

FTATClueBucketKey UTATNPCClueSpawnerComponent::GetClueBucket() const
{
   return FTATClueBucketKey {
      .Type = ETATClueType::NPC,
      .PlacementTag = PlacementTag,
   };
}

void UTATNPCClueSpawnerComponent::AuthorityCacheClue(FTATNPCDialogueClueData&& newClue)
{
   if (!newClue.IsValid())
   {
      UE_LOG(LogTATNPCClueSpawnerComponent, Error, TEXT("[%s] | AuthoritySetClue() called with invalid clue!"), *GetOwner()->GetName());
      return;
   }

   // Should only be called once
   check(!_clue.IsValid());

   check(GetOwner()->HasAuthority());

   _clue = MoveTemp(newClue);

   OnSpawnerClueCachedNative.ExecuteIfBound();
   OnSpawnerClueCached.Broadcast();
}
