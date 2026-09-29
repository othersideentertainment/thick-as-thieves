// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// ose
#include "AI/Perception/OSEAISenseSharedConfigData.h"

// ue4
#include "GameplayTagAssetInterface.h"
#include "Perception/AIPerceptionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAISenseSharedConfigData)

DECLARE_CYCLE_STAT(TEXT("OSE Perception SharedConfig: Gameplay Tag Perception Modifiers"), STAT_OSE_AI_SharedConfig_GameplayTagPerceptionModifiers, STATGROUP_AI);

float UOSEAISenseSharedConfigData::CalculateRangePerceptionModifiers(const UAIPerceptionComponent* listener) const
{
   SCOPE_CYCLE_COUNTER(STAT_OSE_AI_SharedConfig_GameplayTagPerceptionModifiers);
   check(listener);
   if(RangePerceptionModifiers.Num() == 0)
   {
      return 1.f;
   }
   float returnValue = 1.f;
   if(const AController* listenerController = Cast<AController>(listener->GetOwner()))
   {
      if(const IGameplayTagAssetInterface* abilitySystemInterface = Cast<IGameplayTagAssetInterface>(listenerController->GetPawn()))
      {
         for (auto it = RangePerceptionModifiers.CreateConstIterator(); it; ++it)
         {
            if(abilitySystemInterface->HasMatchingGameplayTag(it->GameplayTag))
            {
               returnValue *= it->ModifierIfGameplayTagPresent;
            }
         }
      }
   }
   return returnValue;
}
