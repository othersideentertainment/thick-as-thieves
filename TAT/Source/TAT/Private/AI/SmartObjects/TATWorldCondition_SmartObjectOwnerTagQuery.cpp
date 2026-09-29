// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/SmartObjects/TATWorldCondition_SmartObjectOwnerTagQuery.h"

// tat
#include "AI/SmartObjects/TATSmartObjectTagInterface.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "SmartObjectTypes.h"
#include "WorldConditionContext.h"
#include "VisualLogger/VisualLogger.h"
#include "WorldConditions/SmartObjectWorldConditionSchema.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWorldCondition_SmartObjectOwnerTagQuery)

#define LOCTEXT_NAMESPACE "TATSmartObjects"

#if WITH_EDITOR
FText FTATWorldCondition_SmartObjectOwnerTagQuery::GetDescription() const
{
   return LOCTEXT("ActorTagQueryDesc", "[TAT] Match SmartObject Owner Tags");
}
#endif // WITH_EDITOR

bool FTATWorldCondition_SmartObjectOwnerTagQuery::Initialize(const UWorldConditionSchema& schema)
{
   const USmartObjectWorldConditionSchema* smartObjectSchema = Cast<USmartObjectWorldConditionSchema>(&schema);
   if (smartObjectSchema == nullptr)
   {
      UE_LOG(LogSmartObject, Error, TEXT("[%s] Expecting schema based on %s."), ANSI_TO_TCHAR(__FUNCTION__), *USmartObjectWorldConditionSchema::StaticClass()->GetName());
      return false;
   }

   SmartObjectActorRef = smartObjectSchema->GetSmartObjectActorRef();

   bCanCacheResult = schema.GetContextDataTypeByRef(SmartObjectActorRef) == EWorldConditionContextDataType::Persistent;

   return true;
}

bool FTATWorldCondition_SmartObjectOwnerTagQuery::Activate(const FWorldConditionContext& context) const
{
   if (!SmartObjectActorRef.IsValid())
   {
      UE_VLOG_UELOG(context.GetOwner(), LogWorldCondition, Error, TEXT("[%s] The provided 'SmartObjectActorRef' is not set! Owner: %s ; SmartObjectActorRef: %s"),
         ANSI_TO_TCHAR(__FUNCTION__), *GetNameSafe(context.GetOwner()), *SmartObjectActorRef.GetName().ToString());

      return false;
   }

   // FIXME: avoid const-cast, but they don't expose a non-const one, and the interface method is non-const
   AActor* const smartObjectActor = const_cast<AActor*>(context.GetContextDataPtr<AActor>(SmartObjectActorRef));

   if (TagQuery.IsEmpty())
   {
      UE_VLOG_UELOG(context.GetOwner(), LogWorldCondition, Error, TEXT("[%s] The provided 'TagQuery' is empty! Owner: %s ; SmartObjectActor: %s"),
         ANSI_TO_TCHAR(__FUNCTION__), *GetNameSafe(context.GetOwner()), *GetNameSafe(smartObjectActor));

      return false;
   }

   if (bCanCacheResult)
   {
      if (ITATSmartObjectTagInterface* smartObjectTags = Cast<ITATSmartObjectTagInterface>(smartObjectActor))
      {
         FStateType& state = context.GetState(*this);
         ensure(!state.DelegateHandle.IsValid());

         // TODO: copying what unreal is doing, but it might be nice to only listen for relevant tag changes rather than filter in the listent
         state.DelegateHandle = smartObjectTags->GetGameplayTagCountContainer().RegisterGenericGameplayEvent().AddLambda([this, invalidationHandle = context.GetInvalidationHandle(*this)](const FGameplayTag inTag, int32)
            {
               // Get the list of all unique gameplay tags referenced by the query so we can invalidate our result if the added/removed tag is one of them.
               TArray<FGameplayTag> queryTags;
               TagQuery.GetGameplayTagArray(queryTags);
               if (queryTags.Contains(inTag))
               {
                  invalidationHandle.InvalidateResult();
               }
            });
      }
      else
      {
         UE_VLOG_UELOG(context.GetOwner(), LogWorldCondition, Error,
            TEXT("[%s] The provided 'SmartObjectActor' does not implement ITATSmartObjectTagInterface. Owner: %s ; SmartObjectActor: %s"),
            ANSI_TO_TCHAR(__FUNCTION__), *GetNameSafe(context.GetOwner()), *GetNameSafe(smartObjectActor));

         return false;
      }
   }

   return true;
}

FWorldConditionResult FTATWorldCondition_SmartObjectOwnerTagQuery::IsTrue(const FWorldConditionContext& context) const
{
   // FIXME: avoid const-cast, but they don't expose a non-const one, and the interface method is non-const
   AActor* const smartObjectActor = const_cast<AActor*>(context.GetContextDataPtr<AActor>(SmartObjectActorRef));

   FStateType& state = context.GetState(*this);
   const bool resultCanBeCached = state.DelegateHandle.IsValid();
   FWorldConditionResult result(EWorldConditionResultValue::IsFalse, resultCanBeCached);
   if (ITATSmartObjectTagInterface* tagInterface = Cast<ITATSmartObjectTagInterface>(smartObjectActor))
   {
      if (TagQuery.Matches(tagInterface->GetGameplayTagCountContainer().GetExplicitGameplayTags()))
      {
         result.Value = EWorldConditionResultValue::IsTrue;
      }
   }

   return result;
}

void FTATWorldCondition_SmartObjectOwnerTagQuery::Deactivate(const FWorldConditionContext& context) const
{
   FStateType& state = context.GetState(*this);

   if (state.DelegateHandle.IsValid())
   {
      // FIXME: avoid const-cast, but they don't expose a non-const one, and the interface method is non-const
      AActor* const smartObjectActor = const_cast<AActor*>(context.GetContextDataPtr<AActor>(SmartObjectActorRef));
      if (ITATSmartObjectTagInterface* tagInterface = Cast<ITATSmartObjectTagInterface>(smartObjectActor))
      {
         tagInterface->GetGameplayTagCountContainer().RegisterGenericGameplayEvent().Remove(state.DelegateHandle);
      }
      state.DelegateHandle.Reset();
   }
}

#undef LOCTEXT_NAMESPACE
