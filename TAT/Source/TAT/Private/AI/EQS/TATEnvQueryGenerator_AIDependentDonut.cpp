// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/EQS/TATEnvQueryGenerator_AIDependentDonut.h"

#include "AI/TATAIController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEnvQueryGenerator_AIDependentDonut)

void UTATEnvQueryGenerator_AIDependentDonut::ProjectAndFilterNavPoints(
   TArray<FNavLocation>& points,
   FEnvQueryInstance& queryInstance) const
{
   if ( const ATATAIController* tatAIController = Cast<ATATAIController>(queryInstance.Owner.Get()))
   {
      // if the AI controller is flagged to not use path finding, just return true on everything so this becomes a no-op
      if (tatAIController->ShouldForceMoveRequestsToBlockPathfinding())
      {
         return;
      }
   }
   Super::ProjectAndFilterNavPoints(points, queryInstance);
}
