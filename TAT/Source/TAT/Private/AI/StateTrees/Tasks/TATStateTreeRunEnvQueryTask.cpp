// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#include "AI/StateTrees/Tasks/TATStateTreeRunEnvQueryTask.h"

// ue
#include "EnvironmentQuery/EnvQuery.h"
#include "EnvironmentQuery/EnvQueryManager.h"
#include "StateTreeExecutionContext.h"
#include "EnvQueryItemType_SmartObject.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeRunEnvQueryTask)

// Taken from https://github.com/EpicGames/UnrealEngine/blob/585df42eb3a391efd295abd231333df20cddbcf3/Engine/Plugins/Runtime/StateTree/Source/StateTreeModule/Public/StateTreePropertyRef.h#L4
// However modified heavily to work in our version of UE
EStateTreeRunStatus FTATStateTreeRunEnvQueryTask::EnterState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
	FInstanceDataType& instanceData = context.GetInstanceData(*this);
	if (!instanceData.QueryTemplate)
	{
		return EStateTreeRunStatus::Failed;
	}

	FEnvQueryRequest request(instanceData.QueryTemplate, instanceData.QueryOwner);
   // Note we can only send Int's, Floats and Bools into the query config.
	for (FAIDynamicParam& dynamicParam : instanceData.QueryConfig)
	{
		request.SetDynamicParam(dynamicParam, nullptr);
	}

	instanceData.RequestId = request.Execute(instanceData.RunMode,
		FQueryFinishedSignature::CreateLambda([InstanceDataRef = context.GetInstanceDataStructRef(*this)](const TSharedPtr<FEnvQueryResult>& queryResult) mutable
			{
				if (FInstanceDataType* instanceData = InstanceDataRef.GetPtr())
				{
					instanceData->QueryResult = queryResult;
					instanceData->RequestId = INDEX_NONE;
				}
			}));
	return instanceData.RequestId != INDEX_NONE ? EStateTreeRunStatus::Running : EStateTreeRunStatus::Failed;
}

EStateTreeRunStatus FTATStateTreeRunEnvQueryTask::Tick(FStateTreeExecutionContext& context, const float deltaTime) const
{
	FInstanceDataType& instanceData = context.GetInstanceData(*this);
	if (instanceData.QueryResult)
	{
		if (instanceData.QueryResult->IsSuccessful())
		{
		   // This is where we diverge from the unreal 5.5 version of the EQS query, which uses a bunch of functions we don't have to dynamically bind
		   // to multiple output types, we only support vectors for now.
		   if(AActor** actorPointer = instanceData.ResultActor.GetMutablePtr<AActor*>(context))
		   {
		      *actorPointer = instanceData.QueryResult->GetItemAsActor(0);
		   }
         if(FVector* vectorPointer = instanceData.ResultVector.GetMutablePtr<FVector>(context))
		   {
		      *vectorPointer = instanceData.QueryResult->GetItemAsLocation(0);
		   }
		   if(FSmartObjectSlotHandle* smartObjectSlotHandle = instanceData.SmartObjectSlotHandle.GetMutablePtr<FSmartObjectSlotHandle>(context))
		   {
		      if (instanceData.QueryResult->ItemType->IsChildOf(UEnvQueryItemType_SmartObject::StaticClass()))
		      {
		         const FSmartObjectSlotEQSItem& item = UEnvQueryItemType_SmartObject::GetValue(instanceData.QueryResult->GetItemRawMemory(0));
		         if(item.SlotHandle.IsValid())
		         {
		            *smartObjectSlotHandle = item.SlotHandle;
		         }
		         else
		         {
		            // got an invalid slot handle back, fail.
		            return EStateTreeRunStatus::Failed;
		         }
		      }
		      else
		      {
		         // Expected a smart object handle, got the wrong type back.
		         return EStateTreeRunStatus::Failed;
		      }
		   }
			return EStateTreeRunStatus::Succeeded;
		}
		else
		{
			return EStateTreeRunStatus::Failed;
		}
	}
	return EStateTreeRunStatus::Running;
}

void FTATStateTreeRunEnvQueryTask::ExitState(FStateTreeExecutionContext& context, const FStateTreeTransitionResult& transition) const
{
	FInstanceDataType& instanceData = context.GetInstanceData(*this);
	if (instanceData.RequestId != INDEX_NONE)
	{
		if (UEnvQueryManager* queryManager = UEnvQueryManager::GetCurrent(context.GetOwner()))
		{
			queryManager->AbortQuery(instanceData.RequestId);
		}
		instanceData.RequestId = INDEX_NONE;
	}
	instanceData.QueryResult.Reset();
}

#if WITH_EDITOR
void FTATStateTreeRunEnvQueryTask::PostEditInstanceDataChangeChainProperty(const FPropertyChangedChainEvent& propertyChangedEvent, const FStateTreeDataView instanceDataView)
{
   if (propertyChangedEvent.GetPropertyName() == GET_MEMBER_NAME_CHECKED(FTATStateTreeRunEnvQueryInstanceData, QueryTemplate))
   {
      FInstanceDataType& instanceData = instanceDataView.GetMutable<FInstanceDataType>();
      if (instanceData.QueryTemplate)
      {
         instanceData.QueryTemplate->CollectQueryParams(*instanceData.QueryTemplate, instanceData.QueryConfig);
         for (FAIDynamicParam& dynamicParam : instanceData.QueryConfig)
         {
            dynamicParam.bAllowBBKey = false;
         }
      }
      else
      {
         instanceData.QueryConfig.Reset();
      }
   }
   else if (propertyChangedEvent.Property->GetFName() == GET_MEMBER_NAME_CHECKED(FAIDynamicParam, bAllowBBKey))
   {
      FInstanceDataType& instanceData = instanceDataView.GetMutable<FInstanceDataType>();
      const int32 changedIndex = propertyChangedEvent.GetArrayIndex(GET_MEMBER_NAME_CHECKED(FTATStateTreeRunEnvQueryInstanceData, QueryConfig).ToString());
      if (instanceData.QueryConfig.IsValidIndex(changedIndex))
      {
         if (!instanceData.QueryConfig[changedIndex].bAllowBBKey)
         {
            instanceData.QueryConfig[changedIndex].BBKey.InvalidateResolvedKey();
         }
      }
   }
}
#endif // WITH_EDITOR

