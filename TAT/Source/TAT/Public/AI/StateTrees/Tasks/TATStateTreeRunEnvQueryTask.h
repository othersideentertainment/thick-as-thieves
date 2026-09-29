// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc.
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

// Taken from https://github.com/EpicGames/UnrealEngine/blob/585df42eb3a391efd295abd231333df20cddbcf3/Engine/Plugins/Runtime/GameplayStateTree/Source/GameplayStateTreeModule/Public/Tasks/StateTreeRunEnvQueryTask.h
// Modified to make work in our version of UE, I've also renamed it to TATStateTreeRunEnvQueryInstance so that we can make use of the new version of the function if we do the 5.5 upgrade.
#pragma once

// ue
#include "StateTreeTaskBase.h"
#include "EnvironmentQuery/EnvQueryTypes.h"
#include "StateTreePropertyRef.h"

#include "TATStateTreeRunEnvQueryTask.generated.h"

USTRUCT()
struct FTATStateTreeRunEnvQueryInstanceData
{
	GENERATED_BODY()
   
   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/Engine.Actor", Optional))
   FStateTreePropertyRef ResultActor;
   
   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/CoreUObject.Vector", Optional))
   FStateTreePropertyRef ResultVector;
   
   UPROPERTY(EditAnywhere, meta = (RefType = "/Script/SmartObjectsModule.SmartObjectSlotHandle", Optional))
   FStateTreePropertyRef SmartObjectSlotHandle;

	// The query will be run with this actor has the owner object.
	UPROPERTY(EditAnywhere, Category = Context)
	TObjectPtr<AActor> QueryOwner = nullptr;

	// The query template to run
	UPROPERTY(EditAnywhere, Category = Parameter)
	TObjectPtr<UEnvQuery> QueryTemplate;

	// Query config associated with the query template.
	UPROPERTY(EditAnywhere, EditFixedSize, Category = Parameter)
	TArray<FAIDynamicParam> QueryConfig;

	/** determines which item will be stored (All = only first matching) */
	UPROPERTY(EditAnywhere, Category = Parameter)
	TEnumAsByte<EEnvQueryRunMode::Type> RunMode = EEnvQueryRunMode::SingleResult;

	TSharedPtr<FEnvQueryResult> QueryResult = nullptr;

	int32 RequestId = INDEX_NONE;
};

/**
* Task that runs an async environment query and outputs the result to an outside parameter. Supports Actor and vector types EQS.
* The task is usually run in a sibling state to the result user will be with the data being stored in the parent state's parameters.
* - Parent (Has an EQS result parameter)
*	- Run Env Query (If success go to Use Query Result)
*	- Use Query Result
*/
USTRUCT(meta = (DisplayName = "Run Env Query", Category = "TAT|AI|EQS"))
struct FTATStateTreeRunEnvQueryTask : public FStateTreeTaskCommonBase
{
	GENERATED_BODY()

	using FInstanceDataType = FTATStateTreeRunEnvQueryInstanceData;

	virtual const UStruct* GetInstanceDataType() const override { return FInstanceDataType::StaticStruct(); }

	virtual EStateTreeRunStatus EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;
	virtual EStateTreeRunStatus Tick(FStateTreeExecutionContext& context, const float deltaTime) const override;
	virtual void ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const override;

#if WITH_EDITOR
	virtual void PostEditInstanceDataChangeChainProperty(const FPropertyChangedChainEvent& propertyChangedEvent, FStateTreeDataView instanceDataView) override;
#endif // WITH_EDITOR
};
