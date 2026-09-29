// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/Clues/TATNPCDialogueClue.h"

// tat
#include "Variation/Clues/TATClueFact.h"
#include "Variation/Clues/TATClueTextUtils.h"
#include "Variation/Clues/TATClueSpawnUtils.h"
#include "Variation/Clues/TATNPCClueSpawnerComponent.h"
#include "Variation/Clues/Effects/TATClueEffectHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATNPCDialogueClue)

FTATClueBucketKey FTATNPCDialogueClue::GetClueBucket() const
{
   return FTATClueBucketKey { 
      .Type = ETATClueType::NPC, 
      .PlacementTag = PlacementTag
   };
}

void FTATNPCDialogueClue::ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const
{
   const FText resolvedClueText = TATClueTextUtils::FormatUsingContext(ClueText, context, spawner);
   const FTATSharedClueFactThunk factThunk = FTATClueFactThunk::Make(Facts, context);
   
   CastChecked<UTATNPCClueSpawnerComponent>(spawner)->AuthorityCacheClue(FTATNPCDialogueClueData{
      .InteractionPrompt = UseOverrideInteractionPrompt ? OverrideInteractionPrompt : FText::GetEmpty(),
      .ClueDialogue = resolvedClueText,
      .FollowUpDialogue = AllowFollowUpDialogue ? TATClueTextUtils::FormatUsingContext(FollowUpDialogue, context, spawner) : FText::GetEmpty(),
      .SourceTag = context.SourceTag,
      .Facts = factThunk,
      .Effects = Effects
   });
}

#if WITH_EDITOR
void FTATNPCDialogueClue::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(ClueText.IsEmpty())
   {
      reportError(INVTEXT("ClueText is empty"));
   }
   if (FollowUpDialogue.IsEmpty() && AllowFollowUpDialogue)
   {
      reportError(INVTEXT("FollowUpDialogue is empty! Enter text or disable via checkbox"));
   }
   
   ClueFactUtils::ValidateFactHandles(Facts, reportError);
   TATClueEffectHelpers::ValidateEffects(Effects, reportError);
}
#endif // WITH_EDITOR
