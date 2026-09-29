// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/EnvironmentQuery/Generator/EnvQueryGenerator_ConditionalRoleAISet.h"

// tat
#include "AI/Reactions/TATAIReactionCoordinator.h"
#include "AI/Reactions/TATAIReactionRole.h"

// ue
#include "EnvironmentQuery/Items/EnvQueryItemType_Actor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(EnvQueryGenerator_ConditionalRoleAISet)
DEFINE_LOG_CATEGORY_STATIC(LogEnvQueryGenerator_ConditionalRoleAISet, Log, All);

#define LOCTEXT_NAMESPACE "EnvQueryGenerator_ConditionalRoleAISet"

UEnvQueryGenerator_ConditionalRoleAISet::UEnvQueryGenerator_ConditionalRoleAISet(
   const FObjectInitializer& objectInitializer)
{
   ItemType = UEnvQueryItemType_Actor::StaticClass();
}

void UEnvQueryGenerator_ConditionalRoleAISet::GenerateItems(FEnvQueryInstance& queryInstance) const
{
   if (UTATAIReactionCoordinatorSubsystem* airc = GetWorld()->GetSubsystem<UTATAIReactionCoordinatorSubsystem>())
   {
      if (const FTATAIReactionRole::FRegistrationRoster* roster = airc->RetrieveRoleRosterForQuery(queryInstance.QueryID))
      {
         for (const TWeakObjectPtr<const ATATCharacterAIBase>& aiCharacterPtr : roster->Registrants)
         {
            if (const ATATCharacterAIBase* aiCharacter = aiCharacterPtr.Get())
            {
               queryInstance.AddItemData<UEnvQueryItemType_Actor>(aiCharacter);
            }
         }
         for (const TWeakObjectPtr<const ATATCharacterAIBase>& aiCharacterPtr : roster->Overflow)
         {
            if (const ATATCharacterAIBase* aiCharacter = aiCharacterPtr.Get())
            {
               queryInstance.AddItemData<UEnvQueryItemType_Actor>(aiCharacter);
            }
         }
      }
      else
      {
         UE_LOG(LogEnvQueryGenerator_ConditionalRoleAISet, Error, TEXT("ConditionalRoleAISet generator failed to retrieve \
            the associated role roster!"));
      }
   }
   else
   {
      UE_LOG(LogEnvQueryGenerator_ConditionalRoleAISet, Error, TEXT("ConditionalRoleAISet generator failed to find the \
            AIReactionCoordinator subsystem!"));
   }
}

FText UEnvQueryGenerator_ConditionalRoleAISet::GetDescriptionTitle() const
{
   return LOCTEXT("EnvQueryGenerator_ConditionalRoleAISet_Title", "Generate items for reaction role registrants and overflow");
}

#undef LOCTEXT_NAMESPACE
