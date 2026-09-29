// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Perception/StimInfo.h"
#include "AI/Utility/UtilityAIStateTarget.h"
#include "AI/Utility/UtilityAITokenOwner.h"
#include "AI/Utility/UtilityAITypes.h"
#include "Math/OSEMathFunctionLibrary.h"

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SmartObjectSubsystem.h"

// self
#include "ConsiderationInput.generated.h"

class AOSEAIController;
class AOSECharacterBase;
class UUtilityAIComponent;
class UUtilityAIStateBase;

DECLARE_STATS_GROUP(TEXT("AI Consideration"), STATGROUP_AIConsideration, STATCAT_Advanced);

#define CONSIDERATION_SCOPE(CONSIDERATION_NAME) DECLARE_SCOPE_CYCLE_COUNTER(TEXT(#CONSIDERATION_NAME), STAT_ConsiderationInput_ ## CONSIDERATION_NAME , STATGROUP_AIConsideration)

namespace OSEConsiderationFixup
{
   UConsiderationInput* FixupConsideration(UConsiderationInput* input, UObject* outerAsset);
}

USTRUCT(BlueprintType)
struct OSEAI_API FConsiderationContext
{
   GENERATED_BODY()

public:

   FConsiderationContext() {}

   FConsiderationContext(AOSEAIController* aiController, AOSECharacterBase* character, const UUtilityAIComponent* utilityComponent, const FUtilityStateTarget& target, const UUtilityAIStateBase* stateInstance, const bool isInitialStateEvaluation)
      : AIController(aiController)
      , Character(character)
      , UtilityComponent(utilityComponent)
      , Target(target)
      , StateInstance(stateInstance)
      , IsInitialStateEvaluation(isInitialStateEvaluation)
   {
   }

   /// Controller for the owning AI
   UPROPERTY(BlueprintReadOnly)
   const AOSEAIController* AIController = nullptr;

   /// Character for the owning AI
   UPROPERTY(BlueprintReadOnly)
   AOSECharacterBase* Character = nullptr;

   /// Utility component for the consideration evaluation
   UPROPERTY(BlueprintReadOnly)
   const UUtilityAIComponent* UtilityComponent = nullptr;

   /// Target for this consideration
   UPROPERTY(BlueprintReadOnly)
   FUtilityStateTarget Target;

   /// The state we're evaluating this consideration in the context of.
   /// (could be the currently running state, could be another state we're evaluating)
   UPROPERTY(BlueprintReadOnly)
   const UUtilityAIStateBase* StateInstance = nullptr;

   /// Is this an initial evaluation to move us into a new state, or a re-evaluation of the same state?
   /// This allows us to write considerations with better heuristics (eg 10m distance enters a new state, but 20m is required to leave it)
   UPROPERTY(BlueprintReadOnly)
   bool IsInitialStateEvaluation = false;
};

UCLASS(Abstract, BlueprintType, EditInlineNew)
class OSEAI_API UConsiderationInput : public UObject
{
   GENERATED_BODY()

public:

   static const FVector2D kOutputRange;

   UConsiderationInput() {}

   virtual float GetValue(const FConsiderationContext& ctx) const PURE_VIRTUAL(UConsiderationInput::GetValue, return 0.0f;);
};

UCLASS(Blueprintable, Abstract, BlueprintType, EditInlineNew)
class OSEAI_API UConsiderationInput_BlueprintBase : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UFUNCTION(BlueprintNativeEvent, Category = "AI|OSE|Utility", DisplayName = "GetValue")
   float GetValue_BP(const FConsiderationContext& ctx) const;
   virtual float GetValue_BP_Implementation(const FConsiderationContext& ctx) const { return 0.0f; }
};

UCLASS(Abstract)
class OSEAI_API UConsiderationInput_Composite : public UConsiderationInput
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Category = "Composite Considerations")
   TArray<FBehaviorConsideration> Considerations;

   virtual float GetValue(const FConsiderationContext& ctx) const override;

protected:
   virtual float _GetCompositeValue(const TArrayView<float>& results) const { unimplemented(); return 0.0f; }
};

// Results in "OR" behavior
UCLASS()
class OSEAI_API UConsiderationInput_Composite_Max : public UConsiderationInput_Composite
{
   GENERATED_BODY()

protected:
   virtual float _GetCompositeValue(const TArrayView<float>& results) const override;
};

// Results in "AND" behavior
UCLASS()
class OSEAI_API UConsiderationInput_Composite_NonZero : public UConsiderationInput_Composite
{
   GENERATED_BODY()

protected:
   virtual float _GetCompositeValue(const TArrayView<float>& results) const override;
};

UCLASS()
class OSEAI_API UConsiderationInput_MyHealth : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class OSEAI_API UConsiderationInput_HasGameplayTag : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag GameplayTag;
};

UCLASS()
class OSEAI_API UConsiderationInput_HasAllGameplayTags : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer GameplayTags;
};

UCLASS()
class OSEAI_API UConsiderationInput_HasAnyGameplayTags : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer GameplayTags;
};

UCLASS()
class OSEAI_API UConsiderationInput_HasAnyNotActionableTargetTags : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class OSEAI_API UConsiderationInput_IsOnCooldown : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer CooldownTags;
};

UCLASS()
class OSEAI_API UConsiderationInput_TimeSinceGameplayTagAdded : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag GameplayTag;
   UPROPERTY(EditDefaultsOnly)
   float MinTime = 0.0f;
   UPROPERTY(EditDefaultsOnly)
   float MaxTime = 1.0f;
   UPROPERTY(EditDefaultsOnly)
   bool UseMaxOutputIfWithinRange = false;
};

UCLASS()
class OSEAI_API UConsiderationInput_TimeSinceGameplayTagRemoved : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag GameplayTag;
   UPROPERTY(EditDefaultsOnly)
   float MinTime = 0.0f;
   UPROPERTY(EditDefaultsOnly)
   float MaxTime = 1.0f;
   UPROPERTY(EditDefaultsOnly)
   bool UseMaxOutputIfWithinRange = false;
};

UCLASS()
class OSEAI_API UConsiderationInput_HasAbility : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer AbilityTags;
};

UCLASS()
class OSEAI_API UConsiderationInput_IsAbilityActive : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer AbilityTags;
};

UCLASS()
class OSEAI_API UConsiderationInput_HasTool : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag ToolCategory;
};

UCLASS()
class OSEAI_API UConsiderationInput_HasToolEquipped : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag ToolCategory;
};

UCLASS()
class OSEAI_API UConsiderationInput_TargetHasTool : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag ToolCategory;
};

UCLASS()
class OSEAI_API UConsiderationInput_TargetHasToolEquipped : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag ToolCategory;
};

UCLASS()
class OSEAI_API UConsiderationInput_TargetHealth : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class OSEAI_API UConsiderationInput_TargetHasHealth : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class OSEAI_API UConsiderationInput_TargetHasGameplayTag : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag GameplayTag;
};

UCLASS()
class OSEAI_API UConsiderationInput_TargetHasAllGameplayTags : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer GameplayTags;
};

UCLASS()
class OSEAI_API UConsiderationInput_TargetHasAnyGameplayTags : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer GameplayTags;
};

UCLASS()
class OSEAI_API UConsiderationInput_HasAccessToAITokenForTarget : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FOSEAITokenInfo TokenInfo;
};

UCLASS()
class OSEAI_API UConsiderationInput_TargetTimeSinceGameplayTagAdded : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag GameplayTag;
   UPROPERTY(EditDefaultsOnly)
   float MinTime = 0.0f;
   UPROPERTY(EditDefaultsOnly)
   float MaxTime = 1.0f;
   UPROPERTY(EditDefaultsOnly)
   bool UseMaxOutputIfWithinRange = false;
};

UCLASS()
class OSEAI_API UConsiderationInput_TargetTimeSinceGameplayTagRemoved : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag GameplayTag;
   UPROPERTY(EditDefaultsOnly)
   float MinTime = 0.0f;
   UPROPERTY(EditDefaultsOnly)
   float MaxTime = 1.0f;
   UPROPERTY(EditDefaultsOnly)
   bool UseMaxOutputIfWithinRange = false;
};

UCLASS()
class OSEAI_API UConsiderationInput_TargetIsAbilityActive : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer AbilityTags;
};

UCLASS()
class OSEAI_API UConsiderationInput_HaveLOSToTarget : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class OSEAI_API UConsiderationInput_NavMeshDistanceToTarget : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Min = 0.0f;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Max = 0.0f;

   // should > Max range be full strength or zero?  Useful for capping a range at which a behavior can execute
   UPROPERTY(EditDefaultsOnly)
   bool IsFullStrengthAtGreaterThanMax = true;
};

UCLASS()
class OSEAI_API UConsiderationInput_NavMeshDistanceToTargetComparison : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float ComparisonValue = 0.0f;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
};

UCLASS()
class OSEAI_API UConsiderationInput_NavMeshDistanceAndCostToTargetComparison : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float PathLengthComparisonValue = 0.0f;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod PathLengthComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float PathCostComparisonValue = 0.0f;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod PathCostComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
};

UCLASS()
class OSEAI_API UConsiderationInput_DirectDistanceToTargetComparison : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float ComparisonValue = 0.0f;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
};

UCLASS()
class OSEAI_API UConsiderationInput_HeightToTarget : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float MaxHeight = 1000.0f;
};

UCLASS()
class OSEAI_API UConsiderationInput_StimConsiderationBase : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

protected:
   virtual float _GetStimValue(const FConsiderationContext& ctx) const { unimplemented(); return 0.0f; }
};

UCLASS()
class OSEAI_API UConsiderationInput_StimInvestigationState : public UConsiderationInput_StimConsiderationBase
{
   GENERATED_BODY()

protected:
   virtual float _GetStimValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EStimInvestigationState InvestigationState = EStimInvestigationState::NotInvestigated;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::EqualTo;
};

UCLASS()
class OSEAI_API UConsiderationInput_StimInvestigatingActorIsSelf : public UConsiderationInput_StimConsiderationBase
{
   GENERATED_BODY()

protected:
   virtual float _GetStimValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class OSEAI_API UConsiderationInput_StimSeverity : public UConsiderationInput_StimConsiderationBase
{
   GENERATED_BODY()

protected:
   virtual float _GetStimValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EStimSeverity StimSeverity = EStimSeverity::Light;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::EqualTo;
};

UCLASS()
class OSEAI_API UConsiderationInput_StimType : public UConsiderationInput_StimConsiderationBase
{
   GENERATED_BODY()

protected:
   virtual float _GetStimValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EStimType StimType = EStimType::Audio;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::EqualTo;
};


UCLASS()
class OSEAI_API UConsiderationInput_IsStimFresh : public UConsiderationInput_StimConsiderationBase
{
   GENERATED_BODY()
   
protected:
   virtual float _GetStimValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float MaxAge = 0.0f;
};

UCLASS()
class OSEAI_API UConsiderationInput_TimeSinceStimReceived : public UConsiderationInput_StimConsiderationBase
{
   GENERATED_BODY()
   
protected:
   virtual float _GetStimValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Min = 0.0f;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Max = 0.0f;
};

UCLASS()
class OSEAI_API UConsiderationInput_DurationInCurrentBehavior : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
   
   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Min = 0.0f;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Max = 0.0f;
};

UCLASS()
class OSEAI_API UConsiderationInput_TimeSinceThisBehaviorMostRecentlyStarted : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
   
   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Min = 0.0f;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Max = 0.0f;
};

UCLASS()
class OSEAI_API UConsiderationInput_TimeSinceThisBehaviorMostRecentlyEnded : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
   
   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Min = 0.0f;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Max = 0.0f;
};

UCLASS()
class OSEAI_API UConsiderationInput_IsSearchNodeOnCooldown : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};
