// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/ConsiderationInput.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Abilities/Attributes/AttributeBaseInterface.h"
#include "AI/Nodes/OSESearchNode.h"
#include "AI/OSEAIController.h"
#include "AI/OSEAISettings.h"
#include "AI/OSEAIFunctionLibrary.h"
#include "AI/OSEKnowledgeComponent.h"
#include "AI/Perception/OSEAIPerceptionComponent.h"
#include "AI/Perception/OSEAISense_Sight.h"
#include "AI/Perception/OSEStimDatabaseInterface.h"
#include "AI/Perception/OSEStimDatabase.h"
#include "AI/Utility/ResponseCurve.h"
#include "AI/Utility/UtilityAIBehavior.h"
#include "AI/Utility/UtilityAIBehaviorComponent.h"
#include "Character/OSECharacterBase.h"
#include "Items/ToolSetInterface.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "SmartObjectComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Perception/AISense_Sight.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ConsiderationInput)

#if WITH_RECAST
#include "NavMesh/RecastNavMesh.h"
#endif

const FVector2D UConsiderationInput::kOutputRange = FVector2D(0.0f, 1.0f);

UConsiderationInput* OSEConsiderationFixup::FixupConsideration(UConsiderationInput* input, UObject* outerAsset)
{
   /*
   if (UConsiderationInput_DistanceToTargetLessThan* oldInput = Cast<UConsiderationInput_DistanceToTargetLessThan>(input))
   {
      UConsiderationInput_NavMeshDistanceToTargetComparison* newInput = NewObject<UConsiderationInput_NavMeshDistanceToTargetComparison>(outerAsset);
      newInput->ComparisonValue = oldInput->Max;
      newInput->ComparisonMethod = EOSEComparisonMethod::LessThan;
      return newInput;
   }
   */

   return input;
}

float UConsiderationInput_BlueprintBase::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(BlueprintConsideration);
   return GetValue_BP(ctx);
}

float UConsiderationInput_Composite::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(Composite);

   // doubt it'll ever be more than 8...?
   TArray<float, TInlineAllocator<8>> results;
   results.Reserve(Considerations.Num());

   for(const FBehaviorConsideration& consideration : Considerations)
   {
      if (!consideration.Input)
         continue;

      if (!consideration.Enabled)
         continue;

      const float inputValue = consideration.Input->GetValue(ctx);
      const float inputScore = consideration.ResponseCurve ? consideration.ResponseCurve->GetFloatValue(inputValue) : inputValue;
      results.Emplace(inputScore);
   }

   return _GetCompositeValue(results);
}

float UConsiderationInput_Composite_Max::_GetCompositeValue(const TArrayView<float>& results) const
{
   float maxResult = 0.0f;
   for(float result : results)
   {
      maxResult = FMath::Max(maxResult, result);
   }
   return maxResult;
}

float UConsiderationInput_Composite_NonZero::_GetCompositeValue(const TArrayView<float>& results) const
{
   if (results.Num() == 0.0f)
      return 1.0f;

   const UOSEAISettings& settings = UOSEAISettings::Get();
   for (float result : results)
   {
      if (result < settings.MinStateScore)
         return 0.0f;
   }

   return 1.0f;   
}

float UConsiderationInput_MyHealth::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(MyHealth);
   if (!ctx.Character)
   {
      return 0.0f;
   }
   return ctx.Character->GetHealthPercent();
}

float UConsiderationInput_HasGameplayTag::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasGameplayTag);
   const UAbilitySystemComponent* asc = ctx.Character ? ctx.Character->GetAbilitySystemComponent() : nullptr;
   if (!asc)
   {
      return 0.0f;
   }

   return asc->HasMatchingGameplayTag(GameplayTag) ? 1.0f : 0.0f;
}

float UConsiderationInput_HasAllGameplayTags::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasAllGameplayTags);
   const UAbilitySystemComponent* asc = ctx.Character ? ctx.Character->GetAbilitySystemComponent() : nullptr;
   if (!asc)
   {
      return 0.0f;
   }

   return asc->HasAllMatchingGameplayTags(GameplayTags) ? 1.0f : 0.0f;
}

float UConsiderationInput_HasAnyGameplayTags::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasAnyGameplayTags);
   const UAbilitySystemComponent* asc = ctx.Character ? ctx.Character->GetAbilitySystemComponent() : nullptr;
   if (!asc)
   {
      return 0.0f;
   }

   return asc->HasAnyMatchingGameplayTags(GameplayTags) ? 1.0f : 0.0f;
}

float UConsiderationInput_HasAnyNotActionableTargetTags::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasAnyNotActionableTargetTags);
   const UAbilitySystemComponent* asc = ctx.Character ? ctx.Character->GetAbilitySystemComponent() : nullptr;
   if (!asc)
   {
      return 0.0f;
   }
   const UOSEAISettings& settings = UOSEAISettings::Get();
   return asc->HasAnyMatchingGameplayTags(settings.NotActionableTargetActorTags) ? 1.0f : 0.0f;
}

float UConsiderationInput_IsOnCooldown::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(IsOnCooldown);
   const UAbilitySystemComponent* asc = ctx.Character ? ctx.Character->GetAbilitySystemComponent() : nullptr;
   if (!asc)
   {
      return 0.0f;
   }

   const FGameplayEffectQuery query = FGameplayEffectQuery::MakeQuery_MatchAnyOwningTags(CooldownTags);

   float endTime = 0.0f;
   float duration = 0.0f;
   if (asc->GetActiveEffectsEndTimeAndDuration(query, endTime, duration))
   {
      if(duration <= UGameplayEffect::INFINITE_DURATION)
      {
         // Infinite duration gameplay effects always return -1 on their duration, so we can be on cooldown FOREVER
         return 1.f;
      }
      return duration > 0.0f ? 1.0f : 0.0f;
   }

   return 0.0f;
}

float UConsiderationInput_TimeSinceGameplayTagAdded::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TimeSinceGameplayTagAdded);
   if (const UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(ctx.Character))
   {
      float timeSinceAdded = float(INDEX_NONE);
      if (asc->GetTimeSinceTagAdded(GameplayTag, timeSinceAdded))
      {
         if (UseMaxOutputIfWithinRange)
         {
            return FMath::IsWithin(timeSinceAdded, MinTime, MaxTime) ? 1.0f : 0.0f;
         }
         else
         {
            return FMath::GetMappedRangeValueClamped(FVector2D(MinTime, MaxTime), kOutputRange, timeSinceAdded);
         }
      }
   }
   return 0.0f;
}

float UConsiderationInput_TimeSinceGameplayTagRemoved::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TimeSinceGameplayTagRemoved);
   if (const UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(ctx.Character))
   {
      float timeSinceRemoved = float(INDEX_NONE);
      if (asc->GetTimeSinceTagRemoved(GameplayTag, timeSinceRemoved))
      {
         if (UseMaxOutputIfWithinRange)
         {
            return FMath::IsWithin(timeSinceRemoved, MinTime, MaxTime) ? 1.0f : 0.0f;
         }
         else
         {
            return FMath::GetMappedRangeValueClamped(FVector2D(MinTime, MaxTime), kOutputRange, timeSinceRemoved);
         }
      }
   }
   return 0.0f;
}

float UConsiderationInput_HasAbility::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasAbility);
   if (const UAbilitySystemComponent* asc = ctx.Character ? ctx.Character->GetAbilitySystemComponent() : nullptr)
   {
      for (const FGameplayAbilitySpec& ability : asc->GetActivatableAbilities())
      {
         if (ability.Ability && ability.Ability->GetAssetTags().HasAny(AbilityTags))
            return 1.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_IsAbilityActive::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(IsAbilityActive);
   if (const UAbilitySystemComponent* asc = ctx.Character ? ctx.Character->GetAbilitySystemComponent() : nullptr)
   {
      for (const FGameplayAbilitySpec& ability : asc->GetActivatableAbilities())
      {
         if (ability.IsActive() && ability.Ability && ability.Ability->GetAssetTags().HasAny(AbilityTags))
            return 1.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_HasTool::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasTool);
   TScriptInterface<IToolSetInterface> toolSet = ctx.Character ? ctx.Character->GetToolSetInterface() : nullptr;
   if (!toolSet)
   {
      return 0.0f;
   }

   return toolSet->HasToolCategory(ToolCategory) ? 1.0f : 0.0f;
}

float UConsiderationInput_HasToolEquipped::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HasToolEquipped);
   TScriptInterface<IToolSetInterface> toolSet = ctx.Character ? ctx.Character->GetToolSetInterface() : nullptr;
   if (!toolSet)
   {
      return 0.0f;
   }

   return toolSet->HasToolEquipped(ToolCategory) ? 1.0f : 0.0f;
}

float UConsiderationInput_TargetHasTool::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetHasTool);
   auto target = Cast<AOSECharacterBase>(ctx.Target.Actor.Get());
   if (TScriptInterface<IToolSetInterface> toolSet = target->GetToolSetInterface())
   {
      return toolSet->HasToolCategory(ToolCategory) ? 1.0f : 0.0f;
   }
   return 0.0f;
}

float UConsiderationInput_TargetHasToolEquipped::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetHasToolEquipped);
   auto target = Cast<AOSECharacterBase>(ctx.Target.Actor.Get());
   if (TScriptInterface<IToolSetInterface> toolSet = target->GetToolSetInterface())
   {
      return toolSet->HasToolEquipped(ToolCategory) ? 1.0f : 0.0f;
   }
   return 0.0f;
}

float UConsiderationInput_TargetHealth::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetHealth);
   const IAttributeBaseInterface* target = Cast<IAttributeBaseInterface>(ctx.Target.Actor.Get());
   if (!target)
   {
      return 0.0f;
   }

   return target->GetHealthPercent();
}

float UConsiderationInput_TargetHasHealth::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetHasHealth);
   const IAttributeBaseInterface* target = Cast<IAttributeBaseInterface>(ctx.Target.Actor.Get());
   if (!target)
   {
      return 0.0f;
   }

   return (target->GetHealthPercent() > 0.0f) ? 1.0f : 0.0f;
}

float UConsiderationInput_TargetHasGameplayTag::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetHasGameplayTag);
   const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(ctx.Target.GetTargetUObject());
   if (!tagInterface)
   {
      return 0.0f;
   }

   return tagInterface->HasMatchingGameplayTag(GameplayTag) ? 1.0f : 0.0f;
}


float UConsiderationInput_TargetHasAllGameplayTags::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetHasAllGameplayTags);

   const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(ctx.Target.GetTargetUObject());
   if (!tagInterface)
   {
      return 0.0f;
   }

   return tagInterface->HasAllMatchingGameplayTags(GameplayTags) ? 1.0f : 0.0f;
}

float UConsiderationInput_TargetHasAnyGameplayTags::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetHasAnyGameplayTags);

   const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(ctx.Target.GetTargetUObject());
   if (!tagInterface)
   {
      return 0.0f;
   }

   return tagInterface->HasAnyMatchingGameplayTags(GameplayTags) ? 1.0f : 0.0f;
}

float UConsiderationInput_HasAccessToAITokenForTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetHasAIToken);
   return ctx.UtilityComponent->HasAccessToAITokenForTarget(TokenInfo, ctx.Target) ? 1.0f : 0.0f;
}

float UConsiderationInput_TargetTimeSinceGameplayTagAdded::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetTimeSinceGameplayTagAdded);
   if (const UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(ctx.Target.Actor.Get()))
   {
      float timeSinceAdded = float(INDEX_NONE);
      if (asc->GetTimeSinceTagAdded(GameplayTag, timeSinceAdded))
      {
         if (UseMaxOutputIfWithinRange)
         {
            return FMath::IsWithin(timeSinceAdded, MinTime, MaxTime) ? 1.0f : 0.0f;
         }
         else
         {
            return FMath::GetMappedRangeValueClamped(FVector2D(MinTime, MaxTime), kOutputRange, timeSinceAdded);
         }
      }
   }
   return 0.0f;
}

float UConsiderationInput_TargetTimeSinceGameplayTagRemoved::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetTimeSinceGameplayTagRemoved);
   if (const UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(ctx.Target.Actor.Get()))
   {
      float timeSinceRemoved = float(INDEX_NONE);
      if (asc->GetTimeSinceTagRemoved(GameplayTag, timeSinceRemoved))
      {
         if (UseMaxOutputIfWithinRange)
         {
            return FMath::IsWithin(timeSinceRemoved, MinTime, MaxTime) ? 1.0f : 0.0f;
         }
         else
         {
            return FMath::GetMappedRangeValueClamped(FVector2D(MinTime, MaxTime), kOutputRange, timeSinceRemoved);
         }
      }
   }
   return 0.0f;
}

float UConsiderationInput_TargetIsAbilityActive::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TargetIsAbilityActive);
   if (const UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(ctx.Target.Actor.Get()))
   {
      for (const FGameplayAbilitySpec& ability : asc->GetActivatableAbilities())
      {
         if (ability.IsActive() && ability.Ability && ability.Ability->GetAssetTags().HasAny(AbilityTags))
            return 1.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_HaveLOSToTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HaveLOSToTarget);
   AActor* target = ctx.Target.Actor.Get();
   if (!target)
   {
      return 0.0f;
   }

   UOSEAIPerceptionComponent* perception = ctx.AIController ? ctx.AIController->GetOSEAIPerceptionComponent() : nullptr;
   const bool perceived = perception ? perception->IsActorCurrentlyPerceived(target, UOSEAISense_Sight::StaticClass()) : false;
   return perceived ? 1.0f : 0.0f;
}

float UConsiderationInput_NavMeshDistanceToTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(NavMeshDistanceToTarget);

   if (!ctx.AIController)
      return 1.0f;

   FOSEAINavMeshCalcPathResult result = UOSEAIFunctionLibrary::CalcNavMeshPathLengthFromCurrentLocation(ctx.AIController, ctx.Target.GetTargetWorldLocation());
   if (!result.HasPartialOrFullPath())
   {
      result.PathLength = Max + 1.0;
   }
   if (!IsFullStrengthAtGreaterThanMax && result.PathLength > Max)
      return 0.0f;
   return FMath::GetMappedRangeValueClamped(FVector2D(Min, Max), kOutputRange, result.PathLength);
}

float UConsiderationInput_NavMeshDistanceToTargetComparison::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(NavMeshDistanceToTargetComparison);
   if (const AOSEAIController* aiController = ctx.AIController)
   {
      if (const AActor* targetActor = ctx.Target.Actor.Get())
      {
         FOSEAINavMeshCalcPathResult result = UOSEAIFunctionLibrary::CalcNavMeshPathLengthFromCurrentLocation(ctx.AIController, targetActor->GetActorLocation());
         if (result.HasPartialOrFullPath())
         {
            return UOSEMathFunctionLibrary::CompareFloats(result.PathLength, ComparisonValue, ComparisonMethod) ? 1.0f : 0.0f;
         }
         return 0.0f;
      }
   }
   return 1.0f;
}

float UConsiderationInput_NavMeshDistanceAndCostToTargetComparison::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(NavMeshDistanceAndCostToTargetComparison);
   if (const AOSEAIController* aiController = ctx.AIController)
   {
      const FVector targetWorldLocation = ctx.Target.GetTargetWorldLocation();
      if (targetWorldLocation != FAISystem::InvalidLocation)
      {
         FOSEAINavMeshCalcPathAndCostResult result = UOSEAIFunctionLibrary::CalcNavMeshPathLengthAndCostFromCurrentLocation(ctx.AIController, targetWorldLocation);
         if (result.HasPartialOrFullPath())
         {
            const bool isPathLengthInRange = UOSEMathFunctionLibrary::CompareFloats(result.PathLength, PathLengthComparisonValue, PathLengthComparisonMethod);
            const bool isPathCostInRange = UOSEMathFunctionLibrary::CompareFloats(result.PathCost, PathCostComparisonValue, PathCostComparisonMethod);
            return (isPathLengthInRange && isPathCostInRange) ? 1.0f : 0.0f;
         }
      }
   }
   return 0.0f;
}

float UConsiderationInput_DirectDistanceToTargetComparison::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(DirectDistanceToTargetComparison);
   const FVector targetWorldLocation = ctx.Target.GetTargetWorldLocation();
   if (ctx.Character && targetWorldLocation != FAISystem::InvalidLocation)
   {
      const FVector characterWorldLocation = ctx.Character->GetActorLocation();      
      const float distSq = FVector::DistSquared(characterWorldLocation, targetWorldLocation);
      return UOSEMathFunctionLibrary::CompareFloats(distSq, FMath::Square(ComparisonValue), ComparisonMethod) ? 1.0f : 0.0f;
   }
   return 0.0f;
}

float UConsiderationInput_HeightToTarget::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(HeightToTarget);
   AActor* target = ctx.Target.Actor.Get();
   if (!ctx.Character || !target)
   {
      return 0.0f;
   }

   const float heightSq = FMath::Square(ctx.Character->GetActorLocation().Z - target->GetActorLocation().Z);
   return FMath::GetMappedRangeValueClamped(FVector2D(0.0f, FMath::Square(MaxHeight)), kOutputRange, heightSq);
}

float UConsiderationInput_StimConsiderationBase::GetValue(const FConsiderationContext& ctx) const
{
   if (!ctx.AIController || !ctx.UtilityComponent)
      return 0.0f;

   if (ctx.Target.TargetType != EBehaviorTargetType::Stim)
      return 0.0f;

   // NOTE: This is the copied stim struct that this is operating on, individual considerations that want to check "live" state
   // like investigations need to look up that info in the db and cannot just act on this info
   return _GetStimValue(ctx);
}

float UConsiderationInput_StimInvestigationState::_GetStimValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(StimInvestigationState);
   
   if (const IOSEStimDatabaseInterface* dbOwner = Cast<IOSEStimDatabaseInterface>(ctx.AIController))
   {
      if (const UOSEStimDatabase* db = dbOwner->AuthorityGetStimDatabase())
      {
         const FStimInfo* liveStimInfo = db->FindStimInfo(ctx.Target.Stim.Id);
         check(liveStimInfo); // should exist since we used this same DB to feed targets to utility states...
         return UOSEMathFunctionLibrary::CompareInts((int)liveStimInfo->InvestigationState, (int)InvestigationState, ComparisonMethod) ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_StimInvestigatingActorIsSelf::_GetStimValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(StimInvestigatingActorIsSelf);

   if (const IOSEStimDatabaseInterface* dbOwner = Cast<IOSEStimDatabaseInterface>(ctx.AIController))
   {
      if (const UOSEStimDatabase* db = dbOwner->AuthorityGetStimDatabase())
      {
         const FStimInfo* liveStimInfo = db->FindStimInfo(ctx.Target.Stim.Id);
         ensure(liveStimInfo); // should exist since we used this same DB to feed targets to utility states...
         return liveStimInfo->InvestigatingActor == ctx.Character ? 1.0f : 0.0f;
      }
   }
   return 0.0f;
}

float UConsiderationInput_StimSeverity::_GetStimValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(StimSeverity);
   return UOSEMathFunctionLibrary::CompareInts((int)ctx.Target.Stim.Severity, (int)StimSeverity, ComparisonMethod) ? 1.0f : 0.0f;
}

float UConsiderationInput_StimType::_GetStimValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(StimSeverity);
   return UOSEMathFunctionLibrary::CompareInts((int)ctx.Target.Stim.Type, (int)StimType, ComparisonMethod) ? 1.0f : 0.0f;
}

float UConsiderationInput_IsStimFresh::_GetStimValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(IsStimFresh);
   const float now = ctx.AIController->GetWorld()->GetTimeSeconds();
   const bool isCurrentTarget = ctx.UtilityComponent->GetCurrentTarget() == ctx.Target;
   const float age = now - ctx.Target.Stim.Timestamp;
   const bool isFresh = isCurrentTarget || age < MaxAge;
   return isFresh ? 1.0f : 0.0f;
}

float UConsiderationInput_TimeSinceStimReceived::_GetStimValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TimeSinceStimReceived);
   const float now = ctx.AIController->GetWorld()->GetTimeSeconds();
   const float stimReceivedTime = ctx.Target.Stim.Timestamp;
   const float timeSinceReceived = FMath::Max(now - stimReceivedTime, 0.0f);
   return FMath::GetMappedRangeValueClamped(FVector2D(Min, Max), kOutputRange, timeSinceReceived);
}

float UConsiderationInput_DurationInCurrentBehavior::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(DurationInCurrentBehavior);

   if (UUtilityAIStateBase* currentState = ctx.UtilityComponent->GetCurrentState())
   {
      const float now = ctx.StateInstance->GetWorld()->GetTimeSeconds();
      const float currentDuration = now - currentState->GetMostRecentStartTime();
      return FMath::GetMappedRangeValueClamped(FVector2D(Min, Max), kOutputRange, currentDuration);
   }
   return 0.0f;
}

float UConsiderationInput_TimeSinceThisBehaviorMostRecentlyStarted::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TimeSinceThisBehaviorMostRecentlyStarted);

   check(ctx.StateInstance);

   const float now = ctx.StateInstance->GetWorld()->GetTimeSeconds();
   const float recentStartTime = ctx.StateInstance->GetMostRecentStartTime();
   const float timeSinceRecentStartTime = FMath::Max(now - recentStartTime, 0.0f);
   return FMath::GetMappedRangeValueClamped(FVector2D(Min, Max), kOutputRange, timeSinceRecentStartTime);
}

float UConsiderationInput_TimeSinceThisBehaviorMostRecentlyEnded::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(TimeSinceCurrentBehaviorMostRecentlyEnded);

   check(ctx.StateInstance);

   if (ctx.StateInstance == ctx.UtilityComponent->GetCurrentState())
   {
      return 0.0f;
   }
   
   const float recentEndTime = ctx.StateInstance->GetMostRecentEndTime();
   const float now = ctx.StateInstance->GetWorld()->GetTimeSeconds();
   const float timeSinceRecentEndTime = FMath::Max(now - recentEndTime, 0.0f);
   return FMath::GetMappedRangeValueClamped(FVector2D(Min, Max), kOutputRange, timeSinceRecentEndTime);
}

float UConsiderationInput_IsSearchNodeOnCooldown::GetValue(const FConsiderationContext& ctx) const
{
   CONSIDERATION_SCOPE(IsSearchNodeOnCooldown);

   if (AOSESearchNode* searchNode = Cast<AOSESearchNode>(ctx.Target.Actor.Get()))
   {
      return searchNode->IsCoolingDown() ? 1.0f : 0.0f;
   }

   return 0.0f;
}

