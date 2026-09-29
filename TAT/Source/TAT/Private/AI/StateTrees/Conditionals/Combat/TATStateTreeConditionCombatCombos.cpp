// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Conditionals/Combat/TATStateTreeConditionCombatCombos.h"

// ue
#include "StateTreeExecutionContext.h"

// ose
#include "Character/OSECharacterBase.h"

// tat
#include "Combat/TATAICombatComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeConditionCombatCombos)

bool FTATStateTreeConditionCombatCombos::TestCondition(FStateTreeExecutionContext& context) const
{
   const FInstanceDataType& instanceData = context.GetInstanceData(*this);

   if(instanceData.Owner == nullptr)
   {
      return false;
   }

   const AOSECharacterBase* characterBase = Cast<AOSECharacterBase>(instanceData.Owner->GetPawn());
   if(characterBase == nullptr)
      return false;

   const UTATAICombatComponent* aiCombatComponent = Cast<UTATAICombatComponent>(characterBase->GetCombatComponent());
   if(aiCombatComponent == nullptr)
      return false;

   const FGameplayTag currentTag = aiCombatComponent->FindCurrentComboAbilityTag();
   return currentTag == instanceData.RequiredComboTag;
}
