// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/BehaviorTree/Decorators/BTDecorator_CheckIfInForcedState.h"

// UE
#include "GameplayTagAssetInterface.h"

// TAT
#include "AI/TATAISettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(BTDecorator_CheckIfInForcedState)

UBTDecorator_CheckIfInForcedState::UBTDecorator_CheckIfInForcedState(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
   NodeName = TEXT("[TAT] Check if AI in Forced State");
   INIT_DECORATOR_NODE_NOTIFY_FLAGS();
}

bool UBTDecorator_CheckIfInForcedState::CalculateRawConditionValue(UBehaviorTreeComponent& ownerComp,
                                                                   uint8* nodeMemory) const
{
   const UTATAISettings& aiSettings = UTATAISettings::Get();
   if(const AController* controller = Cast<AController>(ownerComp.GetOwner()))
   {
      if(const IGameplayTagAssetInterface* gameplayTagAssetInterface = Cast<IGameplayTagAssetInterface>(controller->GetPawn()))
      {
         return gameplayTagAssetInterface->HasAnyMatchingGameplayTags(aiSettings.ForcedStateTags);
      }
   }
   return false;
}
