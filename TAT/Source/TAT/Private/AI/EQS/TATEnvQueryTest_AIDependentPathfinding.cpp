// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/EQS/TATEnvQueryTest_AIDependentPathfinding.h"

#include "AI/TATAIController.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEnvQueryTest_AIDependentPathfinding)

void UTATEnvQueryTest_AIDependentPathfinding::RunTest(FEnvQueryInstance& queryInstance) const
{
   if (const ATATAIController* tatAIController = Cast<ATATAIController>(queryInstance.Owner.Get()))
   {
      // if the AI controller is flagged to not use path finding, just return true on everything so this becomes a no-op
      if (tatAIController->ShouldForceMoveRequestsToBlockPathfinding())
      {
         for (FEnvQueryInstance::ItemIterator It(this, queryInstance); It; ++It)
         {
            if (GetWorkOnFloatValues())
            {
               It.SetScore(TestPurpose, FilterType, 1.f, 1.f,1.f);
            }
            else
            {
               It.SetScore(TestPurpose, FilterType, true, true);
            }
         }
         return;
      }
   }
   
   Super::RunTest(queryInstance);
}

FText UTATEnvQueryTest_AIDependentPathfinding::GetDescriptionTitle() const
{
   return FText::FromString(FString::Printf(TEXT("[TAT] %s"), *Super::GetDescriptionTitle().ToString()));
}
