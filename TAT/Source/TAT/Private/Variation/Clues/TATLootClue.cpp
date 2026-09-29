// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATLootClue.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "UI/Queue/TATUIQueue.h"
#include "Variation/Clues/TATKnownCluesComponent.h"
#include "Variation/Clues/TATReadableClueActor.h"

// ue
#include "GameFramework/Character.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLootClue)

void FTATLootClue::OnLootTaken(ACharacter* interactingCharacter) const
{
   check(interactingCharacter);
   if(interactingCharacter->IsLocallyControlled())
   {
      const TSoftClassPtr<UTATReadableClueWidget>& widgetClass = ReadableWidgetOverride.IsNull() ?
         UTATProjectSettings::Get().DefaultLootClueWidget : ReadableWidgetOverride;
      UTATUIQueue* queue = UTATUIQueue::Create(interactingCharacter->GetController<APlayerController>());
      queue->AddAction(UTATReadableClueWidget::CreateAction(widgetClass, ClueText));
      queue->Run();
   }
   if (interactingCharacter->HasAuthority())
   {
      // For now, skip the source tag until there is a compelling need to wire up to the source
      const FGameplayTag sourceTag = {};
      UTATKnownCluesComponent::BP_RecordKnownClueFacts(interactingCharacter->GetPlayerState(),
         sourceTag,
         Facts);
   }
}

# if WITH_EDITOR
void FTATLootClue::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(ClueText.IsEmpty())
   {
      reportError(INVTEXT("ClueText is empty"));
   }
   ClueFactUtils::ValidateFactHandles(Facts, reportError);
}
#endif
