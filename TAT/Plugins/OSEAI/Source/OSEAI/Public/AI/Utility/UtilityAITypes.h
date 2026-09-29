// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/UtilityAIBehavior.h"
#include "AI/Utility/UtilityAIGoal.h"
#include "AI/Utility/UtilityAITokenOwner.h"

// ue
#include "DrawDebugHelpers.h"

// self
#include "UtilityAITypes.generated.h"

class UConsiderationInput;
class UResponseCurve;
#if ENABLE_VISUAL_LOG
struct FVisualLogEntry;
#endif // ENABLE_VISUAL_LOG

DECLARE_STATS_GROUP(TEXT("Utility AI"), STATGROUP_UtilityAI, STATCAT_Advanced);

DECLARE_LOG_CATEGORY_EXTERN(LogOSEUtilityAI, Log, All);

UENUM(BlueprintType)
enum class EConsiderationEvaluationCondition : uint8
{
   EvaluateInitialOnly,
   EvaluateOngoingOnly,
   EvaluateBoth,
};

USTRUCT()
struct OSEAI_API FBehaviorConsideration
{
   GENERATED_BODY()

public:

   // hidden from the property editor and generated from the name + property set
   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "false", EditConditionHides))
   FString TitlePropertyHidden;

   UPROPERTY(EditDefaultsOnly)
   bool Enabled = true;

   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "Enabled"))
   EConsiderationEvaluationCondition EvaluationCondition = EConsiderationEvaluationCondition::EvaluateBoth;

   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "Enabled"))
   FName Name;

   UPROPERTY(EditDefaultsOnly, Instanced, meta = (EditCondition = "Enabled"))
   UConsiderationInput* Input = nullptr;

   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "Enabled"))
   UResponseCurve* ResponseCurve = nullptr;

   void RefreshTitleProperty();
   bool Fixup(UObject* outerAsset);

   DECLARE_DELEGATE_RetVal_TwoParams(UConsiderationInput*, FOnFixupBehaviorConsiderationInput, UConsiderationInput*, UObject*);
   static FOnFixupBehaviorConsiderationInput OnFixupBehaviorConsiderationInput;
};

// IMPORTANT: This is used as a bitmask and as such we need to maintain the values
// here, serialization does not happen by name, but by int.
UENUM(BlueprintType, meta = (Bitflags))
enum class EUtilityStateTargeting : uint8
{
   None                  = 0,
   Actors                = 1,
   Allies                = 2,
   Enemies               = 3,
   Stims                 = 4,
   // WAS: Interactables = 5
   // WAS: GoalNodes     = 6,
   // WAS: SearchNodes   = 7,
   // WAS: AlarmStations = 8,
   SuspiciousAllies      = 9,
   SquadMembers          = 10,
   // WAS: EscapePoints  = 11,
   SmartObjects          = 12,

   MAX UMETA(Hidden)
};

namespace UtilityTargetingUtl
{
   void ForEachTargetingType(int targetingFlags, const FGameplayTag& targetingGroup, const TFunctionRef<void(EUtilityStateTargeting, const FGameplayTag&)>& cb);
}

UCLASS(Blueprintable, BlueprintType)
class OSEAI_API UUtilityStateEvaluatorSharedConsiderations : public UDataAsset
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable, Category = "Fixup")
   bool FixupConsiderations() { bool ret = false; for (FBehaviorConsideration& consideration : SharedConsiderations) { ret |= consideration.Fixup(this); } return ret; }

   UPROPERTY(EditDefaultsOnly, meta=(TitleProperty="TitlePropertyHidden"))
   TArray<FBehaviorConsideration> SharedConsiderations;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) override;
   virtual void PostLoad() override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostEditUndo() override;
#endif // WITH_EDITOR

private:
#if WITH_EDITOR
   void _RefreshTitleProperty();
#endif // WITH_EDITOR
};

UCLASS(Blueprintable, BlueprintType)
class OSEAI_API UUtilityStateEvaluator : public UDataAsset
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintCallable, Category = "Fixup")
   bool FixupConsiderations() { bool ret = false; for(FBehaviorConsideration& consideration : Considerations) { ret |= consideration.Fixup(this); }  return ret; }

   UPROPERTY()
   EUtilityStateTargeting Targeting_DEPRECATED = EUtilityStateTargeting::None;

   UPROPERTY(EditDefaultsOnly, meta = (Bitmask, BitmaskEnum = "/Script/OSEAI.EUtilityStateTargeting"), DisplayName = "Targeting")
   int TargetingFlags = 0;

   UPROPERTY(EditDefaultsOnly, meta = (Categories = "AI.Behavior.TargetingGroup"))
   FGameplayTag TargetingGroup;

   UPROPERTY(EditDefaultsOnly, meta = (Categories = "AI.Token", TitleProperty = "TokenTag"))
   TArray<FOSEAITokenInfo> TargetTokens;

   UPROPERTY(EditDefaultsOnly)
   TArray<UUtilityStateEvaluatorSharedConsiderations*> SharedConsiderations;

   UPROPERTY(EditDefaultsOnly, meta=(TitleProperty="TitlePropertyHidden"))
   TArray<FBehaviorConsideration> Considerations;

   void ForEachConsideration(const TFunctionRef<void(const FBehaviorConsideration&, bool&)>& cb) const;
   
   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility")
   int GetNumConsiderations() const;

   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility")
   bool HasAnyValidTargetTokens() const;

   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility")
   bool ContainsTokenInfo(const FOSEAITokenInfo& tokenInfo) const;

   // from UObject
   virtual void PostLoad() override;
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostEditUndo() override;
#endif // WITH_EDITOR

private:
#if WITH_EDITOR
   void _RefreshTitleProperty();
#endif // WITH_EDITOR
};

USTRUCT()
struct OSEAI_API FBehaviorSetItem
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   FName Name;

   UPROPERTY(EditDefaultsOnly)
   float Weight = 1.0f;

   /// The bonus weight to give the current state/target pair.
   UPROPERTY(EditDefaultsOnly)
   float MomentumBonus = 0.2f;

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UUtilityAIBehavior> Behavior;

   UPROPERTY(EditDefaultsOnly)
   UUtilityStateEvaluator* Evaluator = nullptr;
};

UCLASS(Blueprintable, BlueprintType)
class OSEAI_API UUtilityBehaviorSet : public UDataAsset
{
   GENERATED_BODY()

public:

   UPROPERTY(EditDefaultsOnly, meta=(TitleProperty=Name), Category = "Behaviors")
   TArray<FBehaviorSetItem> Behaviors;

   UFUNCTION(CallInEditor, Category = "Weights")
   void RefreshWeights() { _RefreshWeights(); }

   // uneditable weights, pulls from the evaluator to display inline
   UPROPERTY(Transient, EditDefaultsOnly, meta = (EditCondition = "false"), Category = "Weights")
   TArray<FString> WeightInfo;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) override;
   virtual void PostLoad() override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostEditUndo() override;
#endif // WITH_EDITOR

private:
   void _RefreshWeights();
};

USTRUCT()
struct OSEAI_API FUtilityGoalSetItem
{
   GENERATED_BODY()

public:
   // hidden from the property editor and generated from the name + property set
   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "false", EditConditionHides))
   FString TitlePropertyHidden;

   UPROPERTY(EditDefaultsOnly)
   FName Name;

   UPROPERTY(EditDefaultsOnly)
   bool Enabled = true;

   UPROPERTY(EditDefaultsOnly)
   float Weight = 1.0f;

   /// The bonus weight to give the current state/target pair.
   UPROPERTY(EditDefaultsOnly)
   float MomentumBonus = 0.2f;

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UUtilityAIGoal> Goal;

   UPROPERTY(EditDefaultsOnly)
   UUtilityStateEvaluator* Evaluator = nullptr;

   void RefreshTitleProperty();
};

UCLASS(Blueprintable, BlueprintType)
class OSEAI_API UUtilityGoalSet : public UDataAsset
{
   GENERATED_BODY()

public:

   UPROPERTY(EditDefaultsOnly, meta=(TitleProperty="TitlePropertyHidden"), Category = "Behaviors")
   TArray<FUtilityGoalSetItem> Goals;

   UFUNCTION(CallInEditor, Category = "Weights")
   void RefreshWeights() { _RefreshWeights(); }

   // uneditable weights, pulls from the evaluator to display inline
   UPROPERTY(Transient, EditDefaultsOnly, meta = (EditCondition = "false"), Category = "Weights")
   TArray<FString> WeightInfo;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) override;
   virtual void PostLoad() override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostEditUndo() override;
#endif // WITH_EDITOR

private:
   void _RefreshWeights();
   void _RefreshTitleProperty();
};

USTRUCT(BlueprintType)
struct OSEAI_API FUtilityStateEvaluatorInstance
{
   GENERATED_BODY()

public:

   FUtilityStateEvaluatorInstance() {}

   FUtilityStateEvaluatorInstance(FName name, float weight, float momentumBonus, UUtilityStateEvaluator* evaluator, UUtilityAIStateBase* instance)
      : Name(name)
      , Weight(weight)
      , MomentumBonus(momentumBonus)
      , Evaluator(evaluator)
      , Instance(instance)
   {
   }

   UPROPERTY(BlueprintReadOnly)
   FName Name;

   UPROPERTY(BlueprintReadOnly)
   float Weight = 0.0f;

   UPROPERTY(BlueprintReadOnly)
   float MomentumBonus = 0.0f;

   UPROPERTY(BlueprintReadOnly)
   UUtilityStateEvaluator* Evaluator = nullptr;

   UPROPERTY(BlueprintReadOnly)
   UUtilityAIStateBase* Instance = nullptr;

   bool operator==(const FUtilityStateEvaluatorInstance& other) const
   {
      return Name == other.Name &&
             Weight == other.Weight &&
             Evaluator == other.Evaluator &&
             Instance == other.Instance;
   }

   bool operator!=(const FUtilityStateEvaluatorInstance& other) const
   {
      return !(*this == other);
   }

   bool IsValid() const
   {
      return Evaluator && Instance;
   }

   static FUtilityStateEvaluatorInstance Invalid;
   static bool CreateAndAdd(UObject& owningObject, UUtilityAIComponent& utilityAIComponent, FName name, float weight, float momentumBonus, UUtilityStateEvaluator* evaluator, TSubclassOf<UUtilityAIStateBase> stateClass, TArray<FUtilityStateEvaluatorInstance>& states);
};

namespace UtilityAIWeightHelper
{
   struct WeightInfo
   {
      FName Name;
      float Weight = 0.0f;

      bool operator<(const WeightInfo& other) const
      {
         if (Weight == other.Weight)
         {
            return Name.LexicalLess(other.Name);
         }
         return Weight < other.Weight;
      }
   };
};
