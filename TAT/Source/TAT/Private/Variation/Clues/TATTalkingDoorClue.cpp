// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATTalkingDoorClue.h"

// tat
#include "Variation/Clues/TATClueSpawnUtils.h"
#include "Variation/Clues/TATTalkingDoor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTalkingDoorClue)

FTATClueBucketKey FTATTalkingDoorClue::GetClueBucket() const
{
   return {ETATClueType::TalkingDoor};
}

void FTATTalkingDoorClue::ApplyToSpawner(UTATClueSpawnerComponent* spawner, const FTATClueContext& context) const
{
   const FTATSharedClueFactThunk clueThunk = FTATClueFactThunk::Make(Facts, context);
   CastChecked<UTATTalkingDoorClueComponent>(spawner)->SetClue({ AudioEvent, context.SourceTag, clueThunk });
}

#if WITH_EDITOR
void FTATTalkingDoorClue::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(AudioEvent.IsNull())
   {
      reportError(INVTEXT("AudioEvent is null"));
   }
   
   ClueFactUtils::ValidateFactHandles(Facts, reportError);
}
#endif
