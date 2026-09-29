// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "StateTreePropertyRef.h"
#include "StateTreeTaskBase.h"
#include "AI/TATAIController.h"

#include "TATStateTreeSendEventToNPC.generated.h"

class AAIController;

USTRUCT()
struct FTATStateTreeInterNPCEvent
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   AActor* FromActor { nullptr };

};

USTRUCT()
struct FTATStateTreeInterNPCEvent_VoiceLine : public FTATStateTreeInterNPCEvent
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere)
   float PreDelayTime { 1.f };

   UPROPERTY(EditAnywhere)
   float PostDelayTime { 1.f };
   
   UPROPERTY(EditAnywhere)
   FGameplayTag ResponseVoLine;
};

USTRUCT()
struct FTATStateTreeInterNPCEvent_ShareTarget : public FTATStateTreeInterNPCEvent_VoiceLine
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere)
   AActor* SharedTargetActor { nullptr };
   
   UPROPERTY(EditAnywhere)
   FVector SharedTargetLocation = FVector::ZeroVector;
};

USTRUCT()
struct FTATStateTreeSendEventToNPCData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category="Context")
   AAIController* AIController { nullptr };

   // Either the character or controller can be supplied.
   UPROPERTY(EditAnywhere, Category="In")
   AActor* TargetNPC { nullptr };
   
   UPROPERTY(EditAnywhere)
   FGameplayTag EventTagToUse;
};


USTRUCT()
struct TAT_API FTATStateTreeSendEventToNPC : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeSendEventToNPCData;
	
   FTATStateTreeSendEventToNPC() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual FInstancedStruct GetEventPayload(FStateTreeExecutionContext& context, ATATAIController* aiController) const;
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};


USTRUCT()
struct FTATStateTreeSendEventToNPC_VoiceLineData : public FTATStateTreeSendEventToNPCData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere)
   FGameplayTag VoiceLineToPlay;

   UPROPERTY(EditAnywhere)
   float DelayBeforeVoiceLine { 1.f };

   UPROPERTY(EditAnywhere)
   float DelayAfterVoiceLine { 1.f };
};


USTRUCT()
struct TAT_API FTATStateTreeSendEventToNPC_VoiceLine : public FTATStateTreeSendEventToNPC
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeSendEventToNPC_VoiceLineData;
	
   FTATStateTreeSendEventToNPC_VoiceLine() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual FInstancedStruct GetEventPayload(FStateTreeExecutionContext& context, ATATAIController* aiController) const override;
};


USTRUCT()
struct FTATStateTreeSendEventToNPC_ShareTargetData : public FTATStateTreeSendEventToNPC_VoiceLineData
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere)
   AActor* TargetToShare { nullptr };

   UPROPERTY(EditAnywhere)
   FVector SharedTargetLocation = FVector::ZeroVector;
};


USTRUCT()
struct TAT_API FTATStateTreeSendEventToNPC_ShareTarget : public FTATStateTreeSendEventToNPC
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeSendEventToNPC_ShareTargetData;
	
   FTATStateTreeSendEventToNPC_ShareTarget() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual FInstancedStruct GetEventPayload(FStateTreeExecutionContext& context, ATATAIController* aiController) const override;
};


USTRUCT()
struct FTATStateTreeExtractEventToNPC_VoiceLineData
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, Category = Context)
   TObjectPtr<AAIController> AIController = nullptr;
   
   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/Engine.Actor"))
   FStateTreePropertyRef FromActor;
   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/GameplayTags.GameplayTag"))
   FStateTreePropertyRef VOLineToPlay;

   // Needs to be of type "Double" because blueprint "float" properties are actually doubles..
   // EdGraphSchema_K2.cpp:4041 (PC_Real instead of PC_Float)
   UPROPERTY(EditAnywhere, meta = (RefType = "double"))
   FStateTreePropertyRef DelayBeforeVoice;
   UPROPERTY(EditAnywhere, meta = (RefType = "double"))
   FStateTreePropertyRef DelayAfterVoice;
};

USTRUCT(meta = (DisplayName = "Extract Conversation Voice Line Data", Category = "TAT|Event|Extractions"))
struct TAT_API FTATStateTreeExtractEventToNPC_VoiceLine : public FStateTreeTaskCommonBase
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeExtractEventToNPC_VoiceLineData;
	
   FTATStateTreeExtractEventToNPC_VoiceLine() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};


USTRUCT()
struct FTATStateTreeExtractEventToNPC_ShareTargetData : public FTATStateTreeExtractEventToNPC_VoiceLineData
{
   GENERATED_BODY()
   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/Engine.Actor", Optional))
   FStateTreePropertyRef SharedTargetActor;
   
   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/CoreUObject.Vector", Optional))
   FStateTreePropertyRef SharedLocationVector;
};

USTRUCT(meta = (DisplayName = "Extract Conversation Share Target Data", Category = "TAT|Event|Extractions"))
struct TAT_API FTATStateTreeExtractEventToNPC_ShareTarget : public FTATStateTreeExtractEventToNPC_VoiceLine
{
   GENERATED_BODY()
   using FInstanceDataType = FTATStateTreeExtractEventToNPC_ShareTargetData;
	
   FTATStateTreeExtractEventToNPC_ShareTarget() = default;
   virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }
   virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
};

