// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/EnvironmentQuery/Context/TATEnvQueryContext_ReactionTarget.h"

// tat
#include "AI/Reactions/TATAIReactionCoordinator.h"

// ue
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "EnvironmentQuery/Items/EnvQueryItemType_Point.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEnvQueryContext_ReactionTarget)
DEFINE_LOG_CATEGORY_STATIC(LogTATEnvQueryContext_ReactionTarget, Log, All);

void UTATEnvQueryContext_ReactionTarget::ProvideContext(FEnvQueryInstance& queryInstance, FEnvQueryContextData& contextData) const
{
   if (UTATAIReactionCoordinatorSubsystem* airc = GetWorld()->GetSubsystem<UTATAIReactionCoordinatorSubsystem>())
   {
      FVector location;
      if (airc->RetrieveTargetLocationForQuery(queryInstance.QueryID, UseProjectedLocation(), location))
      {
         UEnvQueryItemType_Point::SetContextHelper(contextData, location);
      }
      else
      {
         UE_LOG(LogTATEnvQueryContext_ReactionTarget, Error, TEXT("ReactionTarget EQS context failed to find the \
            associated reaction target location!"));
      }
   }
   else
   {
      UE_LOG(LogTATEnvQueryContext_ReactionTarget, Error, TEXT("ReactionTarget EQS context failed to find the \
            AIReactionCoordinator subsystem!"));
   }
}
