// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/SmartObjects/TATSmartObjectStateTreeSchema.h"

// ue
#include "SmartObjectRuntime.h"
#include "SmartObjectSubsystem.h"
#include "StateTreeConditionBase.h"
#include "StateTreeConsiderationBase.h"
#include "StateTreeEvaluatorBase.h"
#include "StateTreePropertyFunctionBase.h"
#include "StateTreeTaskBase.h"

#include "AI/StateTrees/SmartObjects/TATSmartObjectStateTreeTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSmartObjectStateTreeSchema)

UTATSmartObjectStateTreeSchema::UTATSmartObjectStateTreeSchema() : _ContextControllerClass(AAIController::StaticClass()),
   _ContextPawnClass(APawn::StaticClass()),
   _SmartObjectActorClass(AActor::StaticClass()),
   _ContextDataDescs({
      {TAT::SmartObjectStateTree::Names::ContextAIController, AAIController::StaticClass(), FGuid(0x4A8181D, 0x523C4793, 0xBE51F6BA, 0x468C93DF)},
      {TAT::SmartObjectStateTree::Names::ContextPawn, APawn::StaticClass(), FGuid(0x246F891F, 0xC4A54B53, 0x84EBA2C0, 0xA78347D)},
      {TAT::SmartObjectStateTree::Names::SmartObjectActor, AActor::StaticClass(), FGuid(0xBA528EDA, 0x77534C2D, 0x89B0B914, 0x532AA681)},
      {TAT::SmartObjectStateTree::Names::SmartObjectClaimedHandle, FSmartObjectClaimHandle::StaticStruct(),FGuid(0xBA47F5BC, 0x4ACC430A, 0x80AA00EF, 0xBC526C0F)},
      {TAT::SmartObjectStateTree::Names::SlotEntranceHandle, FSmartObjectSlotEntranceHandle::StaticStruct(), FGuid(0xAAED55C5, 0x87734066, 0xAF6314D4, 0x19DE6AA8)},
   })
{
}

bool UTATSmartObjectStateTreeSchema::IsStructAllowed(const UScriptStruct* inScriptStruct) const
{
   return inScriptStruct->IsChildOf(FStateTreeConditionCommonBase::StaticStruct())
   || inScriptStruct->IsChildOf(FStateTreeEvaluatorCommonBase::StaticStruct())
   || inScriptStruct->IsChildOf(FStateTreeTaskCommonBase::StaticStruct())
   || inScriptStruct->IsChildOf(FStateTreeConsiderationCommonBase::StaticStruct())
   || inScriptStruct->IsChildOf(FStateTreePropertyFunctionCommonBase::StaticStruct());
}

bool UTATSmartObjectStateTreeSchema::IsClassAllowed(const UClass* inClass) const
{
   return IsChildOfBlueprintBase(inClass);

}

bool UTATSmartObjectStateTreeSchema::IsExternalItemAllowed(const UStruct& inStruct) const
{
   return inStruct.IsChildOf(AActor::StaticClass())
         || inStruct.IsChildOf(AAIController::StaticClass())
         || inStruct.IsChildOf(UActorComponent::StaticClass())
         || inStruct.IsChildOf(UWorldSubsystem::StaticClass());
}

void UTATSmartObjectStateTreeSchema::PostLoad()
{
   Super::PostLoad();
   _ContextDataDescs[0].Struct = _ContextControllerClass.Get();
   _ContextDataDescs[1].Struct = _ContextPawnClass.Get();
   _ContextDataDescs[2].Struct = _SmartObjectActorClass.Get();
}

#if WITH_EDITOR
void UTATSmartObjectStateTreeSchema::PostEditChangeChainProperty(FPropertyChangedChainEvent& propertyChangedEvent)
{
   Super::PostEditChangeChainProperty(propertyChangedEvent);

   if (const FProperty* property = propertyChangedEvent.Property)
   {
      if (property->GetOwnerClass() == UTATSmartObjectStateTreeSchema::StaticClass()
         && property->GetFName() == GET_MEMBER_NAME_CHECKED(UTATSmartObjectStateTreeSchema, _ContextControllerClass))
      {
         _ContextDataDescs[0].Struct = _ContextControllerClass.Get();
      }
      if (property->GetOwnerClass() == UTATSmartObjectStateTreeSchema::StaticClass()
         && property->GetFName() == GET_MEMBER_NAME_CHECKED(UTATSmartObjectStateTreeSchema, _ContextPawnClass))
      {
         _ContextDataDescs[1].Struct = _ContextPawnClass.Get();
      }
      if (property->GetOwnerClass() == UTATSmartObjectStateTreeSchema::StaticClass()
         && property->GetFName() == GET_MEMBER_NAME_CHECKED(UTATSmartObjectStateTreeSchema, _SmartObjectActorClass))
      {
         _ContextDataDescs[2].Struct = _SmartObjectActorClass.Get();
      }
   }
}
#endif
