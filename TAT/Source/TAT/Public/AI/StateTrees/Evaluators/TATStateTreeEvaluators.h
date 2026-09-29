// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "GameFramework/TATDifficulty.h"

// ue
#include "StateTreeEvaluatorBase.h"

#include "TATStateTreeEvaluators.generated.h"

USTRUCT()
struct FTATDifficultyEvaluatorData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere)
   ETATDifficulty Difficulty { ETATDifficulty::Normal };
   UPROPERTY(EditAnywhere)
   int VigilantLoopCount { 1 };
};

USTRUCT(meta = (DisplayName = "Difficulty Evaluator", Category = "TAT|Difficulty"))
struct TAT_API FTATDifficultyEvaluator : public FStateTreeEvaluatorCommonBase
{
   GENERATED_BODY()
  
   using FInstanceDataType = FTATDifficultyEvaluatorData;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct();}
   
   virtual void TreeStart(FStateTreeExecutionContext& Context) const override;
};
