// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// tat

// ue
#include "CoreMinimal.h"

#include "TATQuestFlowUtils.generated.h"

struct FTATContractInfo;
struct FMatchPersistentQuestResult;
class UTATUIQueue;
struct FGameplayTag;
struct FTATQuestInfo;

UCLASS()
class TAT_API UTATQuestFlowUtils : public UObject
{
   GENERATED_BODY()
   
public:
   // on the edge of wanting a params struct
   static void AddPostMatchContractFlow(UTATUIQueue* queue, const FMatchPersistentQuestResult& questResult);
   static void AddContractCompleteFlow(UTATUIQueue* queue, const FTATContractInfo& quest);
   static void AddContractPreCompleteFlow(UTATUIQueue* queue, const FTATContractInfo& quest);
   static void AddContractPostCompleteFlow(UTATUIQueue* queue, const FTATContractInfo& quest);
   static void AddContractFailedFlow(UTATUIQueue* queue, const FTATContractInfo& quest);
   static void AddMissionCompleteFlow(UTATUIQueue* queue, FGameplayTag questTag, bool didComplete);
};
