// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"

#include "TATStateTreeTaskExtractDistractionEvent.generated.h"

USTRUCT()
struct FTATStateTreeTaskExtractDistractionEventData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, meta=(RefType = "/Script/SmartObjectsModule.SmartObjectSlotHandle"))
   FStateTreePropertyRef SmartObjectHandle;
};

USTRUCT(meta = (DisplayName = "Extract Distraction Data", Category = "TAT|Event|Extractions"))
struct TAT_API FTATStateTreeTaskExtractDistractionEvent : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeTaskExtractDistractionEventData;
	
   FTATStateTreeTaskExtractDistractionEvent() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};
