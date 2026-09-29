// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/UtilityAITypes.h"

// ose
#include "OSECommon.h"
#include "AI/Utility/ConsiderationInput.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAITypes)

DEFINE_LOG_CATEGORY(LogOSEUtilityAI);

//---------------------------------------------------------------------------------------
// FBehaviorConsideration
//---------------------------------------------------------------------------------------

FBehaviorConsideration::FOnFixupBehaviorConsiderationInput FBehaviorConsideration::OnFixupBehaviorConsiderationInput;

void FBehaviorConsideration::RefreshTitleProperty()
{
   TitlePropertyHidden = FString::Printf(TEXT("%s%s%s")
      , Enabled ? TEXT("") : TEXT("[Disabled] ")
      , EvaluationCondition == EConsiderationEvaluationCondition::EvaluateInitialOnly ? TEXT("[Initial] ") : EvaluationCondition == EConsiderationEvaluationCondition::EvaluateOngoingOnly ? TEXT("[Ongoing] ") : TEXT("")
      , *Name.ToString());
}

bool FBehaviorConsideration::Fixup(UObject* outerAsset)
{
   // NOTE: use this as a place to port considerations, since it's not always easy to rename/refactor these via redirects since you often want to copy properties across.
   // This + the editor utility to trigger it offer a convenient place to write some porting code before removing considerations

   // return true when a change is made to mark the object dirty in the editor for re-saving
   bool changed = false;

   // fixup at the ose level
   if (UConsiderationInput* oseInput = OSEConsiderationFixup::FixupConsideration(Input, outerAsset))
   {
      if (oseInput != Input)
      {
         Input = oseInput;
         changed = true;
      }      
   }

   // allow project considerations to hook into fixup
   if (OnFixupBehaviorConsiderationInput.IsBound())
   {
      if (UConsiderationInput* gameInput = OnFixupBehaviorConsiderationInput.Execute(Input, outerAsset))
      {
         if (gameInput != Input)
         {
            Input = gameInput;
            changed = true;
         }
      }
   }

   return changed;
}

//---------------------------------------------------------------------------------------
// UtilityTargetingUtl
//---------------------------------------------------------------------------------------

void UtilityTargetingUtl::ForEachTargetingType(int targetingFlags, const FGameplayTag& targetingGroup, const TFunctionRef<void(EUtilityStateTargeting, const FGameplayTag&)>& cb)
{
   for(int idx = 0; idx < static_cast<int>(EUtilityStateTargeting::MAX); ++idx)
   {
      EUtilityStateTargeting targetingType = static_cast<EUtilityStateTargeting>(idx);
      if ((targetingFlags & UOSECommon::EnumToFlags(targetingType)) != 0)
      {
         cb(targetingType, targetingGroup);
      }
   }
}

//---------------------------------------------------------------------------------------
// FBehaviorEvaluatorInstance
//---------------------------------------------------------------------------------------

/* static */
FUtilityStateEvaluatorInstance FUtilityStateEvaluatorInstance::Invalid = FUtilityStateEvaluatorInstance();

/* static */
bool FUtilityStateEvaluatorInstance::CreateAndAdd(UObject& owningObject, UUtilityAIComponent& utilityAIComponent, FName name, float weight, float momentumBonus, UUtilityStateEvaluator* evaluator, TSubclassOf<UUtilityAIStateBase> stateClass, TArray<FUtilityStateEvaluatorInstance>& states)
{
   if (!stateClass || !evaluator)
   {
      UE_LOG(LogOSEUtilityAI, Warning, TEXT("Cannot create behavior evaluator instance for %s!"), *owningObject.GetName());
      return false;
   }

   // Verify that this Evaluator's Considerations are valid.
   bool validEvaluator = true;
   evaluator->ForEachConsideration(
      [&validEvaluator, &stateClass](const FBehaviorConsideration& consideration, bool& done)
      {
         if (!consideration.Input || !consideration.ResponseCurve)
         {
            UE_LOG(LogOSEUtilityAI, Warning, TEXT("The consideration \"%s\" for behavior \"%s\" is invalid! Ignoring the behavior."), *consideration.Name.ToString(), *stateClass->GetName());
            validEvaluator = false;
            done = true;
         }
      }
   );

   if (!validEvaluator)
   {
      return false;
   }

   // Create a new instance of the state.
   UUtilityAIStateBase* stateInstance = NewObject<UUtilityAIStateBase>(&owningObject, stateClass);
   if (!stateInstance)
   {
      UE_LOG(LogOSEUtilityAI, Error, TEXT("Could not allocate instance for state \"%s\" in %s! Skipping it."), *stateClass->GetName(), *owningObject.GetName());
      return false;
   }
   stateInstance->Init(utilityAIComponent);
   states.Emplace(name, weight, momentumBonus, evaluator, stateInstance);
   return true;
}

//---------------------------------------------------------------------------------------
// UUtilityBehaviorEvaluatorSharedConsiderations
//---------------------------------------------------------------------------------------

#if WITH_EDITOR
EDataValidationResult UUtilityStateEvaluatorSharedConsiderations::IsDataValid(FDataValidationContext& context)
{
   if (SharedConsiderations.Num() == 0)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no shared considerations!"), *GetName())));
   }
   for (int idx = 0; idx < SharedConsiderations.Num(); ++idx)
   {
      const FBehaviorConsideration& consideration = SharedConsiderations[idx];
      if (!consideration.Input)
      {
         context.AddError(
            FText::FromString(FString::Printf(TEXT("[%s] shared consideration at entry %d has no input!"), *GetName(), idx)));
      }
      if (!consideration.ResponseCurve)
      {
         context.AddError(
            FText::FromString(FString::Printf(TEXT("[%s] shared consideration at entry %d has no response curve!"), *GetName(), idx)));
      }
   }
   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

void UUtilityStateEvaluatorSharedConsiderations::PostLoad()
{
   Super::PostLoad();
   _RefreshTitleProperty();
}

void UUtilityStateEvaluatorSharedConsiderations::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   _RefreshTitleProperty();
}

void UUtilityStateEvaluatorSharedConsiderations::PostEditUndo()
{
   Super::PostEditUndo();
   _RefreshTitleProperty();
}

void UUtilityStateEvaluatorSharedConsiderations::_RefreshTitleProperty()
{
   for (FBehaviorConsideration& consideration : SharedConsiderations)
   {
      consideration.RefreshTitleProperty();
   }
}
#endif // WITH_EDITOR

//---------------------------------------------------------------------------------------
// UUtilityBehaviorEvaluator
//---------------------------------------------------------------------------------------

void UUtilityStateEvaluator::ForEachConsideration(const TFunctionRef<void(const FBehaviorConsideration&, bool&)>& cb) const
{
   bool done = false;

   // shared first, assuming they're generic things that will be our fastest early-out
   for(UUtilityStateEvaluatorSharedConsiderations* sharedConsiderations : SharedConsiderations)
   {
      if (!sharedConsiderations)
         continue;

      for(const FBehaviorConsideration& consideration : sharedConsiderations->SharedConsiderations)
      {
         cb(consideration, done);
         if (done)
            return;
      }
   }

   // then specific considerations
   for (const FBehaviorConsideration& consideration : Considerations)
   {
      cb(consideration, done);
      if (done)
         return;
   }
}

int UUtilityStateEvaluator::GetNumConsiderations() const
{
   int numConsiderations = Considerations.Num();
   for (UUtilityStateEvaluatorSharedConsiderations* sharedConsiderations : SharedConsiderations)
   {
      if (!sharedConsiderations)
         continue;

      numConsiderations += sharedConsiderations->SharedConsiderations.Num();
   }
   return numConsiderations;
}

bool UUtilityStateEvaluator::HasAnyValidTargetTokens() const
{
   for(const FOSEAITokenInfo& tokenInfo : TargetTokens)
   {
      if(tokenInfo.IsValid())
      {
         return true;
      }
   }
   return false;
}

bool UUtilityStateEvaluator::ContainsTokenInfo(const FOSEAITokenInfo& tokenInfo) const
{
   return TargetTokens.Contains(tokenInfo);
}

void UUtilityStateEvaluator::PostLoad()
{
   Super::PostLoad();

   // migrate from old enum to new bitmask
   // TODO: remove this once all affected assets have been re-saved
   if (TargetingFlags == 0)
   {
      TargetingFlags = UOSECommon::EnumToFlags(Targeting_DEPRECATED);
   }

#if WITH_EDITOR
   _RefreshTitleProperty();
#endif // WITH_EDITOR
}

#if WITH_EDITOR
EDataValidationResult UUtilityStateEvaluator::IsDataValid(FDataValidationContext& context)
{
   if (GetNumConsiderations() == 0)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no considerations!"), *GetName())));
   }

   for (int idx = 0; idx < Considerations.Num(); ++idx)
   {
      const FBehaviorConsideration& consideration = Considerations[idx];
      if (!consideration.Input)
      {
         context.AddError(
            FText::FromString(FString::Printf(TEXT("[%s] consideration at entry %d has no input!"), *GetName(), idx)));
      }
      if (!consideration.ResponseCurve)
      {
         context.AddError(
            FText::FromString(FString::Printf(TEXT("[%s] consideration at entry %d has no response curve!"), *GetName(), idx)));
      }
   }

   TArray<FGameplayTag> tokenTags;
   for(int idx = 0; idx < TargetTokens.Num(); ++idx)
   {
      const FOSEAITokenInfo& tokenInfo = TargetTokens[idx];
      if (tokenInfo.IsValid())
      {
         if (tokenTags.Contains(tokenInfo.TokenTag))
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("[%s] contains duplicate token tag %s in entry %d!"), *GetName(), *tokenInfo.TokenTag.ToString(), idx)));
         }
         else
         {
            tokenTags.Add(tokenInfo.TokenTag);
         }
      }
      else
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has invalid token info in entry %d!"), *GetName(), idx)));
      }
   }
   if ((TargetingFlags & UOSECommon::EnumToFlags(EUtilityStateTargeting::None)) != 0)
   {
      if (TargetTokens.Num() > 0)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no targeting but some target tokens!"), *GetName())));
      }
   }
   for (int idx = 0; idx < SharedConsiderations.Num(); ++idx)
   {
      UUtilityStateEvaluatorSharedConsiderations* sharedConsiderations = SharedConsiderations[idx];
      if (!sharedConsiderations)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has invalid shared consideration in entry %d!"), *GetName(), idx)));
      }
   }
   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

void UUtilityStateEvaluator::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   _RefreshTitleProperty();
}

void UUtilityStateEvaluator::PostEditUndo()
{
   Super::PostEditUndo();
   _RefreshTitleProperty();
}

void UUtilityStateEvaluator::_RefreshTitleProperty()
{
   for(FBehaviorConsideration& consideration : Considerations)
   {
      consideration.RefreshTitleProperty();
   }
}
#endif // WITH_EDITOR

//---------------------------------------------------------------------------------------
// UUtilityBehaviorSet
//---------------------------------------------------------------------------------------

#if WITH_EDITOR
EDataValidationResult UUtilityBehaviorSet::IsDataValid(FDataValidationContext& context)
{
   for (int idx = 0; idx < Behaviors.Num(); ++idx)
   {
      FBehaviorSetItem& behaviorSet = Behaviors[idx];
      if (!behaviorSet.Evaluator)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no evaluator at index %d!"), *GetName(), idx)));
      }
      if(!behaviorSet.Behavior.Get())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no behavior at index %d!"), *GetName(), idx)));
      }
   }
   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

void UUtilityBehaviorSet::PostLoad()
{
   Super::PostLoad();
   _RefreshWeights();
}

void UUtilityBehaviorSet::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   _RefreshWeights();
}

void UUtilityBehaviorSet::PostEditUndo()
{
   Super::PostEditUndo();
   _RefreshWeights();
}
#endif // WITH_EDITOR

void UUtilityBehaviorSet::_RefreshWeights()
{
#if WITH_EDITOR
   WeightInfo.Reset();

   TArray<UtilityAIWeightHelper::WeightInfo> weightInfos;

   for (FBehaviorSetItem& behaviorSet : Behaviors)
   {
      if (behaviorSet.Evaluator)
      {
         weightInfos.Add({ behaviorSet.Name, behaviorSet.Weight });
      }
   }

   weightInfos.Sort();

   for(const UtilityAIWeightHelper::WeightInfo& info : weightInfos)
   {
      WeightInfo.Add(FString::Printf(TEXT("%.02f: %s"), info.Weight, *info.Name.ToString()));
   }
#endif // WITH_EDITOR
}

//---------------------------------------------------------------------------------------
// FUtilityGoalSetItem
//---------------------------------------------------------------------------------------

void FUtilityGoalSetItem::RefreshTitleProperty()
{
#if WITH_EDITOR
   TitlePropertyHidden = FString::Printf(TEXT("%s%s")
      , Enabled ? TEXT("") : TEXT("[Disabled] ")
      , *Name.ToString());
#endif // WITH_EDITOR
}

//---------------------------------------------------------------------------------------
// UUtilityGoalSet
//---------------------------------------------------------------------------------------

#if WITH_EDITOR
EDataValidationResult UUtilityGoalSet::IsDataValid(FDataValidationContext& context)
{
   for (int idx = 0; idx < Goals.Num(); ++idx)
   {
      FUtilityGoalSetItem& goalSet = Goals[idx];
      if (!goalSet.Evaluator)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no evaluator at index %d!"), *GetName(), idx)));
      }
      if (!goalSet.Goal.Get())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no goal at index %d!"), *GetName(), idx)));
      }
   }
   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}

void UUtilityGoalSet::PostLoad()
{
   Super::PostLoad();
   _RefreshWeights();
   _RefreshTitleProperty();
}

void UUtilityGoalSet::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   _RefreshWeights();
   _RefreshTitleProperty();
}

void UUtilityGoalSet::PostEditUndo()
{
   Super::PostEditUndo();
   _RefreshWeights();
   _RefreshTitleProperty();
}
#endif // WITH_EDITOR

void UUtilityGoalSet::_RefreshWeights()
{
#if WITH_EDITOR
   WeightInfo.Reset();

   TArray<UtilityAIWeightHelper::WeightInfo> weightInfos;

   for (const FUtilityGoalSetItem& goalSet : Goals)
   {
      if (goalSet.Evaluator && goalSet.Enabled)
      {
         weightInfos.Add({ goalSet.Name, goalSet.Weight });
      }
   }

   weightInfos.Sort();

   for (const UtilityAIWeightHelper::WeightInfo& info : weightInfos)
   {
      WeightInfo.Add(FString::Printf(TEXT("%.02f: %s"), info.Weight, *info.Name.ToString()));
   }
#endif // WITH_EDITOR
}

void UUtilityGoalSet::_RefreshTitleProperty()
{
   for (FUtilityGoalSetItem& goalSet : Goals)
   {
      goalSet.RefreshTitleProperty();
   }
}

