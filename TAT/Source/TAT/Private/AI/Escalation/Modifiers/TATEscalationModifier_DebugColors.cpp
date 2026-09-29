// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Escalation/Modifiers/TATEscalationModifier_DebugColors.h"

#include "AI/TATAIController.h"
#include "Character/TATCharacterAIBase.h"
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEscalationModifier_DebugColors)

void SetColorOnHead(const ATATAIController* aiController, FColor color)
{
   // TODO: remove whole class?
}
void UTATEscalationModifier_DebugColors::Apply(ATATAIController* aiController) const
{
   Super::Apply(aiController);
   SetColorOnHead(aiController, ColorToSetOnApply);
}

void UTATEscalationModifier_DebugColors::Revert(ATATAIController* aiController) const
{
   SetColorOnHead(aiController, ColorToSetOnRevert);
   Super::Revert(aiController);
}
