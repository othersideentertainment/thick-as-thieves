// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATReadableClue.h"

// tat
#include "Variation/TATActorDerivedSeed.h"
#include "Variation/Clues/TATClueActorSpawner.h"
#include "Variation/Clues/TATClueFact.h"
#include "Variation/Clues/TATClueSpawnUtils.h"
#include "Variation/Clues/TATClueTextUtils.h"
#include "Variation/Clues/TATClueType.h"
#include "Variation/Clues/TATReadableClueActor.h"
#include "Variation/Clues/TATReadableClueSpawner.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATReadableClue)

namespace ReadableClueHelpers
{
   // could have also just been an instance method, but this keeps it out of the header :shrug:
   static const FText& ChooseText(const FTATReadableClue& clue, const UTATClueSpawnerComponent* spawner)
   {
      if (clue.AlternateClueText.IsEmpty())
      {
         return clue.ClueText;
      }
      
      const int32 seed = TATActorDerivedSeed::GetDerivedSeedForActor(spawner->GetOwner(), TEXT("Clue"));
      FRandomStream randomStream(seed);
      const int32 index = randomStream.RandHelper(clue.AlternateClueText.Num() + 1);
      if (index == 0)
      {
         return clue.ClueText;
      }
      else
      {
         return clue.AlternateClueText[index - 1];
      }
   }
}

FTATClueBucketKey FTATReadableClue::GetClueBucket() const
{
   check(IsValid(Visuals));
   
   return {ETATClueType::Readable, AsClueSubtype(Visuals->ReadableType), PlacementTag};
}

void FTATReadableClue::ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const
{
   check(IsValid(Visuals));

   auto formatText = [allowReplacement = AllowTextReplacement, spawner, &context](const FText& format)
   {
      return allowReplacement ?
         TATClueTextUtils::FormatUsingContext(format, context, spawner) :
         format;
   };

   const FText& chosenText = ReadableClueHelpers::ChooseText(*this, spawner);
   FText resolvedClueText = formatText(chosenText);
   FTATSharedClueFactThunk factThunk = FTATClueFactThunk::Make(Facts, context);
   FTATClueFactNamespace factNamespace = { .SourceTag = context.SourceTag, .SourceIndex = context.SourceIndex};
   
   CastChecked<UTATClueActorSpawnerComponent>(spawner)->SpawnClueActor(Visuals->ReadableActorClass,
      [clueText = MoveTemp(resolvedClueText), facts = MoveTemp(factThunk), factNamespace](ATATReadableClueActor* actor)
      {
         actor->InitClueData({
            .ClueText = clueText,
            .Facts = facts,
            .FactTags = ClueFactUtils::TagsFromThunk(facts.Get()),
            .FactNamespace = factNamespace,
         });
      });
}

FString FTATReadableClue::GetDebugDescription() const
{
   if (Visuals != nullptr && !Visuals->ReadableActorClass.IsNull())
   {
      return TATClueInfoHelpers::FormatDebugDescription(TEXT("Actor"),
         FString::Printf(TEXT("[%s] %s"), *Visuals->ReadableActorClass.GetAssetName(), *TATClueInfoHelpers::MakePreviewText(ClueText)));
   }
   return TATClueInfoHelpers::FormatDebugDescription(TEXT("Actor"), TATClueInfoHelpers::MakePreviewText(ClueText));
}

#if WITH_EDITOR
void FTATReadableClue::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(!Visuals)
   {
      reportError(INVTEXT("Visuals is null"));
   }
   if(!PlacementTag.IsValid())
   {
      reportError(INVTEXT("PlacementTag is empty"));
   }
   if(ClueText.IsEmpty())
   {
      reportError(INVTEXT("ClueText is empty"));
   }
   for (int i = 0; i < AlternateClueText.Num(); i++)
   {
      if (AlternateClueText[i].IsEmpty())
      {
         reportError(FText::FormatOrdered(INVTEXT("AlternateClueText is empty at index {0}"), i));
      }
   }
   ClueFactUtils::ValidateFactHandles(Facts, reportError);
}
#endif
