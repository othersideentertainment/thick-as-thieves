// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/SmartObjects/TATSmartObjectStateTreeContext.h"

// tat
#include "AI/StateTrees/SmartObjects/TATSmartObjectBehaviorDefinition.h"
#include "AI/StateTrees/SmartObjects/TATSmartObjectStateTreeSchema.h"
#include "AI/StateTrees/SmartObjects/TATSmartObjectStateTreeTypes.h"
#include "AI/StateTrees/Tasks/SmartObjects/TATStateTreeSmartObjectTypes.h"

// ue
#include "StateTreeReference.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSmartObjectStateTreeContext)

bool FTATSmartObjectStateTreeContext::Activate(const UTATSmartObjectBehaviorDefinition& definition)
{
   _Definition = &definition;
   check(_Definition)

   const FStateTreeReference& stateTreeReference = _Definition->StateTreeReference;
   const UStateTree* stateTree = stateTreeReference.GetStateTree();

   if(IsValid() == false)
      return false;
   
   if(stateTree == nullptr)
      return false;
   
   FStateTreeExecutionContext stateTreeExecutionContext(*_ContextPawn, *stateTree, _StateTreeInstanceData);
   if(stateTreeExecutionContext.IsValid() == false)
      return false;
   
   if(ValidateSchema(stateTreeExecutionContext) == false)
      return false;
   
   if(SetContextRequirements(stateTreeExecutionContext) == false)
      return false;
   
   USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(_ContextPawn->GetWorld());
   check(smartObjectSubsystem);
   FSmartObjectSlotView slotView = smartObjectSubsystem->GetSlotView(_ClaimedHandle.SlotHandle);
   if (slotView.IsValid())
   {
      if (FTATSmartObjectInteractionSlotUserData* userData = slotView.GetStateDataPtr<FTATSmartObjectInteractionSlotUserData>())
      {
         userData->UserActor = _ContextPawn;
      }
      else
      {
         smartObjectSubsystem->AddSlotData(_ClaimedHandle, FConstStructView::Make(FTATSmartObjectInteractionSlotUserData(_ContextPawn)));
      }
   }

   stateTreeExecutionContext.Start(&stateTreeReference.GetParameters());
   return true;
}

bool FTATSmartObjectStateTreeContext::Tick(const float deltaTime)
{
   if(_Definition == nullptr || IsValid() == false)
      return false;

   const FStateTreeReference& stateTreeReference = _Definition->StateTreeReference;
   const UStateTree* stateTree = stateTreeReference.GetStateTree();
   FStateTreeExecutionContext stateTreeExecutionContext(*_ContextPawn, *stateTree, _StateTreeInstanceData);

   EStateTreeRunStatus runStatus = EStateTreeRunStatus::Unset;
   if (SetContextRequirements(stateTreeExecutionContext))
   {
      runStatus = stateTreeExecutionContext.Tick(deltaTime);
   }
   return runStatus == EStateTreeRunStatus::Running;
}

void FTATSmartObjectStateTreeContext::Deactivate()
{
   if (_Definition == nullptr)
      return;

   const FStateTreeReference& stateTreeReference = _Definition->StateTreeReference;
   const UStateTree* stateTree = stateTreeReference.GetStateTree();
   FStateTreeExecutionContext stateTreeContext(*_ContextPawn, *stateTree, _StateTreeInstanceData);

   if (SetContextRequirements(stateTreeContext))
   {
      stateTreeContext.Stop();
   }
   
   const USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(_ContextPawn->GetWorld());
   check(smartObjectSubsystem);
   FSmartObjectSlotView slotView = smartObjectSubsystem->GetSlotView(_ClaimedHandle.SlotHandle);
   if (slotView.IsValid())
   {
      if (FTATSmartObjectInteractionSlotUserData* userData = slotView.GetStateDataPtr<FTATSmartObjectInteractionSlotUserData>())
      {
         userData->UserActor = nullptr;
      }
   }
}

void FTATSmartObjectStateTreeContext::SendEvent(const FGameplayTag tag, const FConstStructView payload, const FName origin)
{
   if (_Definition == nullptr)
   {
      return;
   }
	
   const FStateTreeReference& stateTreeReference = _Definition->StateTreeReference;
   const UStateTree* stateTree = stateTreeReference.GetStateTree();
   const FStateTreeExecutionContext stateTreeContext(*_ContextPawn, *stateTree, _StateTreeInstanceData);
   stateTreeContext.SendEvent(tag, payload, origin);
}

bool FTATSmartObjectStateTreeContext::SetContextRequirements(FStateTreeExecutionContext& stateTreeContext)
{
	if (stateTreeContext.IsValid() == false)
	{
		return false;
	}

	if (::IsValid(_Definition) == false)
	{
		return false;
	}
	
	stateTreeContext.SetContextDataByName(TAT::SmartObjectStateTree::Names::ContextPawn, FStateTreeDataView(_ContextPawn));
   stateTreeContext.SetContextDataByName(TAT::SmartObjectStateTree::Names::ContextAIController, FStateTreeDataView(_ContextController));
   stateTreeContext.SetContextDataByName(TAT::SmartObjectStateTree::Names::SmartObjectActor, FStateTreeDataView(_SmartObjectActor));
   stateTreeContext.SetContextDataByName(TAT::SmartObjectStateTree::Names::SmartObjectClaimedHandle, FStateTreeDataView(FStructView::Make(_ClaimedHandle)));
	stateTreeContext.SetContextDataByName(TAT::SmartObjectStateTree::Names::SlotEntranceHandle, FStateTreeDataView(FStructView::Make(_SlotEntranceHandle)));

	checkf(_ContextPawn != nullptr, TEXT("Should never reach this point with an invalid ContextActor since it is required to get a valid StateTreeContext."));
	const UWorld* world = _ContextPawn->GetWorld();
	
	stateTreeContext.SetCollectExternalDataCallback(FOnCollectStateTreeExternalData::CreateLambda(
		[world]
		(const FStateTreeExecutionContext& context, const UStateTree* stateTree, const TArrayView<const FStateTreeExternalDataDesc> externalDescs, const TArrayView<FStateTreeDataView> outDataViews)
		{
			check(externalDescs.Num() == outDataViews.Num());
			for (int32 Index = 0; Index < externalDescs.Num(); Index++)
			{
				const FStateTreeExternalDataDesc& Desc = externalDescs[Index];
				if (Desc.Struct != nullptr)
				{
					if (world != nullptr && Desc.Struct->IsChildOf(UWorldSubsystem::StaticClass()))
					{
						UWorldSubsystem* Subsystem = world->GetSubsystemBase(Cast<UClass>(const_cast<UStruct*>(ToRawPtr(Desc.Struct))));
						outDataViews[Index] = FStateTreeDataView(Subsystem);
					}
				}
			}
				
			return true;
		})
	);

	return stateTreeContext.AreContextDataViewsValid();
}

bool FTATSmartObjectStateTreeContext::ValidateSchema(const FStateTreeExecutionContext& stateTreeContext) const
{
   const UTATSmartObjectStateTreeSchema* schema = Cast<UTATSmartObjectStateTreeSchema>(stateTreeContext.GetStateTree()->GetSchema());
   if (schema == nullptr)
   {
      return false;
   }
   if (_ContextPawn == nullptr || _ContextPawn->IsA(schema->GetContextPawnClass()) == false)
   {
      return false;
   }
   if (_ContextController == nullptr || _ContextController->IsA(schema->GetContextControllerClass()) == false)
   {
      return false;
   }
   if (_SmartObjectActor == nullptr || _SmartObjectActor->IsA(schema->GetSmartObjectActorClass()) == false)
   {
      return false;
   }
   return true;
}
