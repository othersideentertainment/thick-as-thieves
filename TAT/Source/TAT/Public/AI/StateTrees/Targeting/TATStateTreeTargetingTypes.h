// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "AI/TATAsyncRequestComponent.h"

// ose 
#include "AI/Alertness/AlertnessEnums.h"
#include "AI/Alertness/DetectionEnums.h"
#include "Character/OSETeamInterface.h"
#include "Math/OSEMathFunctionLibrary.h"

// ue
#include "SmartObjectSubsystem.h"

#include "TATStateTreeTargetingTypes.generated.h"

class UResponseCurve;
class UTATStateTreeTargetingConsiderationInput;
class AOSEAIController;
class AOSECharacterBase;
struct FTATActorKnowledge;
enum class EKnowledgeSource : uint8;

UENUM(BlueprintType)
enum class ETATStateTreeTargetingConsiderationEvaluationCondition : uint8
{
   IgnoreIfCurrentTarget,
   AlwaysEvaluate
};

UENUM()
enum class ETATStateTreeTargetingConsiderationTargetType : uint8
{
   // Consider potential targets from the set of known actors pulled from the AI's knowledge component.
   KnownActors = 1 << 0,
   // Consider all players in the current match as potential targets.
   Players     = 1 << 1,

   // Bitmask helper to cover all entries.
   ALL         = KnownActors | Players UMETA(Hidden)
};
ENUM_CLASS_FLAGS(ETATStateTreeTargetingConsiderationTargetType);

USTRUCT()
struct FTATStateTreeTargetingConsideration
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   bool Enabled = true;

#if WITH_EDITORONLY_DATA
   // Hidden from the property editor and generated from the name + enable state.
   // We can't leave this as UPROPERTY() (without params) as a UPROPERTY needs to be
   // editable in some way to be accessible by metadata specifiers, such as on
   // UTATStateTreeTargetingConsiderations::Considerations 
   // (see PropertyEditorHelpers::ShouldBeVisible())
   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "false", EditConditionHides))
   FString TitlePropertyHidden;

   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "Enabled"))
   FName Name;
#endif // WITH_EDITORONLY_DATA
   
   UPROPERTY(EditDefaultsOnly, Instanced, meta = (EditCondition = "Enabled"))
   UTATStateTreeTargetingConsiderationInput* Input = nullptr;

   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "Enabled"))
   UResponseCurve* ResponseCurve = nullptr;

   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "Enabled"))
   ETATStateTreeTargetingConsiderationEvaluationCondition EvaluationCondition { ETATStateTreeTargetingConsiderationEvaluationCondition::AlwaysEvaluate };

#if WITH_EDITOR
   void RefreshTitleProperty();
#endif // WITH_EDITOR
};

UCLASS()
class TAT_API UTATStateTreeTargetingConsiderations : public UDataAsset
{
   GENERATED_BODY()
public:
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag TargetingTag;

   UPROPERTY(EditDefaultsOnly)
   ETATStateTreeTargetingConsiderationTargetType TargetType = ETATStateTreeTargetingConsiderationTargetType::KnownActors;

   UPROPERTY(EditDefaultsOnly)
   float TargetMomentumAdditionalScore { 0.1f };
   
   UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "TitlePropertyHidden"))
   TArray<FTATStateTreeTargetingConsideration> Considerations;

#if WITH_EDITOR
   // from UObject
   virtual void PostLoad() override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostEditUndo() override;
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;

private:
   void _RefreshTitleProperty();
#endif // WITH_EDITOR
};

USTRUCT()
struct FTATStateTreeTargetingConsiderationTargetContext
{
   GENERATED_BODY()

public:
   // Should be valid for all target types.
   TObjectPtr<AActor> Actor = nullptr;
   double WorldTime { 0 };
private:
   // Should only be valid for KnownActors.
   const FTATActorKnowledge* Knowledge = nullptr;

public:
   FORCEINLINE void SetKnowledge(const FTATActorKnowledge& knowledge) { Knowledge = &knowledge; }
   FORCEINLINE const FTATActorKnowledge& GetKnowledge() const { check(Knowledge != nullptr); return *Knowledge; }
   FORCEINLINE void ClearKnowledge() { Knowledge = nullptr; }

   FORCEINLINE bool IsValidForTargetType(ETATStateTreeTargetingConsiderationTargetType type) const
   {
      switch (type)
      {
         case ETATStateTreeTargetingConsiderationTargetType::KnownActors:
         {
            return (Actor != nullptr) && (Knowledge != nullptr);
         }
         case ETATStateTreeTargetingConsiderationTargetType::Players:
         {
            return (Actor != nullptr);
         }
         case ETATStateTreeTargetingConsiderationTargetType::ALL:
         {
            // If we're valid for all target types, we should require the minimum data
            // inside necessary the target context.
            return (Actor != nullptr);
         }
         default: checkNoEntry(); return false;
      }
   }
};

UCLASS(Abstract, BlueprintType, EditInlineNew)
class TAT_API UTATStateTreeTargetingConsiderationInput : public UObject
{
   GENERATED_BODY()
public:
   using ETargetType = ETATStateTreeTargetingConsiderationTargetType;
   using FTargetContext = FTATStateTreeTargetingConsiderationTargetContext;

   UTATStateTreeTargetingConsiderationInput(){}

   FORCEINLINE bool DoesSupportTargetType(ETargetType type) const
   {
      return EnumHasAnyFlags(GetSupportedTargetTypeBitmask(), type);
   }

   float GetValue(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const;

protected:
   virtual ETargetType GetSupportedTargetTypeBitmask() const { return ETargetType::ALL; }

   bool IsTargetContextValid(const FTargetContext& targetContext) const;

   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const { return 0.f; }
};

UCLASS(Abstract)
class UTATStateTreeTargetingConsiderationInput_Composite : public UTATStateTreeTargetingConsiderationInput
{  
   GENERATED_BODY()
public:
   UPROPERTY(EditDefaultsOnly, meta = (TitleProperty = "TitlePropertyHidden"))
   TArray<FTATStateTreeTargetingConsideration> Considerations;
   virtual ETargetType GetSupportedTargetTypeBitmask() const override { return ETargetType::KnownActors; }
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
protected:
   virtual float _GetCompositeValue(const TArrayView<float>& results) const { unimplemented(); return 0.0f; }
};

// Results in "OR" behavior
UCLASS()
class UTATStateTreeTargetingConsiderationInput_Composite_Max : public UTATStateTreeTargetingConsiderationInput_Composite
{
   GENERATED_BODY()

protected:
   virtual float _GetCompositeValue(const TArrayView<float>& results) const override;
};

// Results in "AND" behavior
UCLASS()
class UTATStateTreeTargetingConsiderationInput_Composite_NonZero : public UTATStateTreeTargetingConsiderationInput_Composite
{
   GENERATED_BODY()

protected:
   virtual float _GetCompositeValue(const TArrayView<float>& results) const override;
};
UCLASS()
class UTATStateTreeTargetingConsiderationInput_IsAttitude : public UTATStateTreeTargetingConsiderationInput
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   EOSETeamAttitude TargetAttitude { EOSETeamAttitude::Neutral };

protected:
   virtual ETargetType GetSupportedTargetTypeBitmask() const override { return ETargetType::KnownActors; }
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_MatchesVisibility : public UTATStateTreeTargetingConsiderationInput
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   bool TargetVisibility { true };

protected:
   virtual ETargetType GetSupportedTargetTypeBitmask() const override { return ETargetType::KnownActors; }
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_Distance : public UTATStateTreeTargetingConsiderationInput
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, meta=(Units="cm"))
   float MaxDistance { 2000.f };
   
   UPROPERTY(EditDefaultsOnly, meta=(Units="cm"))
   float MinDistance { 0.f };

protected:
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
   float CalculateNormalizedValue(float distance) const;
};
UCLASS()
class UTATStateTreeTargetingConsiderationInput_PathDistance : public UTATStateTreeTargetingConsiderationInput_Distance
{
   GENERATED_BODY()

protected:
   virtual ETargetType GetSupportedTargetTypeBitmask() const override { return ETargetType::KnownActors; }
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};


UCLASS()
class UTATStateTreeTargetingConsiderationInput_MatchesTagQuery : public UTATStateTreeTargetingConsiderationInput
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   FGameplayTagQuery TagQuery;

protected:
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};


UCLASS()
class UTATStateTreeTargetingConsiderationInput_HasIndividualKnowledgeOfTarget : public UTATStateTreeTargetingConsiderationInput 
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Category="TAT|Knowledge")
   FGameplayTagQuery QueryToRun;

protected:
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_HasMajorLoot : public UTATStateTreeTargetingConsiderationInput 
{
   GENERATED_BODY()

protected:
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_HasTrait : public UTATStateTreeTargetingConsiderationInput 
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, meta = (Categories = "AI.Trait"))
   FGameplayTag Trait;

protected:
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_HasPathToTarget : public UTATStateTreeTargetingConsiderationInput 
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   bool RequireFullPath { true };

protected:
   virtual ETargetType GetSupportedTargetTypeBitmask() const override { return ETargetType::KnownActors; }
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_CheckNumberOfNavAgents : public UTATStateTreeTargetingConsiderationInput 
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
   
   UPROPERTY(EditDefaultsOnly)
   int NumberOfAgents { 1 };

   UPROPERTY(EditDefaultsOnly)
   bool ShouldFailIfTargetNotNavLinkOwner = false;

protected:
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_AlertnessMatches : public UTATStateTreeTargetingConsiderationInput 
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;

   UPROPERTY(EditDefaultsOnly)
   EAlertnessLevel AlertnessLevel { EAlertnessLevel::Neutral };

protected:
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_DetectionMatches : public UTATStateTreeTargetingConsiderationInput 
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;

   UPROPERTY(EditDefaultsOnly)
   EActorDetectionState DetectionLevel { EActorDetectionState::Observing };

protected:
   virtual ETargetType GetSupportedTargetTypeBitmask() const override { return ETargetType::KnownActors; }
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_KnowledgeSourceMatches : public UTATStateTreeTargetingConsiderationInput
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly)
   EKnowledgeSource KnowledgeSource;

protected:
   virtual ETargetType GetSupportedTargetTypeBitmask() const override { return ETargetType::KnownActors; }
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

// Is the target in a (private) zone that I can access?
UCLASS()
class UTATStateTreeTargetingConsiderationInput_IsInAccessibleZone : public UTATStateTreeTargetingConsiderationInput
{
   GENERATED_BODY()

protected:
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

// Is the target able to interact with me to receive a clue?
UCLASS()
class UTATStateTreeTargetingConsiderationInput_IsClueInteractableByTarget : public UTATStateTreeTargetingConsiderationInput
{
   GENERATED_BODY()

protected:
   virtual ETargetType GetSupportedTargetTypeBitmask() const override { return ETargetType::Players; }
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_LineTraceToTarget : public UTATStateTreeTargetingConsiderationInput
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly)
	bool SucceedIfUnblocked = true;

	UPROPERTY(EditDefaultsOnly)
	ETATAsyncRequestTraceLocationType SourceLocation = ETATAsyncRequestTraceLocationType::EyesViewPoint;

	UPROPERTY(EditDefaultsOnly)
	ETATAsyncRequestTraceLocationType TargetLocation = ETATAsyncRequestTraceLocationType::ActorLocation;

	UPROPERTY(EditDefaultsOnly)
	FCollisionProfileName CollisionProfile;

	// How often, since the last trace, should we trigger a new async trace?
	UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0", Units = "seconds"))
	float TraceInterval = 1.0f;

protected:
	virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_CanInteractWithTarget : public UTATStateTreeTargetingConsiderationInput 
{
   GENERATED_BODY()

protected:
   virtual ETargetType GetSupportedTargetTypeBitmask() const override { return ETargetType::KnownActors; }
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};

UCLASS()
class UTATStateTreeTargetingConsiderationInput_CheckUtilityTargetingGroup : public UTATStateTreeTargetingConsiderationInput
{
   GENERATED_BODY()
public:
   UPROPERTY(EditDefaultsOnly)
   FGameplayTagQuery TagQuery;
protected:
   virtual ETargetType GetSupportedTargetTypeBitmask() const override { return ETargetType::KnownActors; }
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};


UCLASS()
class UTATStateTreeTargetingConsiderationInput_SmartObjectHasSlotAvailable : public UTATStateTreeTargetingConsiderationInput
{
   GENERATED_BODY()
public:
   UPROPERTY(EditDefaultsOnly)
   FSmartObjectRequestFilter Filter;
protected:
   virtual ETargetType GetSupportedTargetTypeBitmask() const override { return ETargetType::KnownActors; }
   virtual float GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const override;
};
