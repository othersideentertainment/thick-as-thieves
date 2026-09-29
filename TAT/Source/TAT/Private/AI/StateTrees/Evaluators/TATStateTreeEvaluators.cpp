// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/StateTrees/Evaluators/TATStateTreeEvaluators.h"

#include "StateTreeExecutionContext.h"

#include "AI/TATAISettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeEvaluators)

void FTATDifficultyEvaluator::TreeStart(FStateTreeExecutionContext& Context) const
{
   FInstanceDataType& instanceData = Context.GetInstanceData(*this);
   
   instanceData.Difficulty = TATDifficulty::GetDifficultyForMatch(Context.GetWorld());
   const UTATAISettings& AISettings = UTATAISettings::Get();
   if (const int* foundLoopCount = AISettings.DifficultyToVigilantLoopCount.Find(instanceData.Difficulty))
   {
      instanceData.VigilantLoopCount = *foundLoopCount;
   }
   else
   {
      instanceData.VigilantLoopCount = 1;
   }
}
