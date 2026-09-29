// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEGameplayAbility.h"
#include "Abilities/Tasks/AbilityTask_EventTimeLine.h"
#include "Combat/CombatTypes.h"
#include "Player/OSEPlayerController.h"

// ue4
#include "Curves/CurveFloat.h"
#include "Engine/DataAsset.h"

#include "OSESyncedAnimations.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogSyncedAnimations, Log, All);

class AOSECharacterBase;
struct FSyncedAnimationEntry;

UENUM(BlueprintType)
enum class ESyncedAnimationAlignmentMode : uint8
{
   TargetForward,
   TargetToSource,
   SourceForward,
   SourceToTarget,
};

UENUM(BlueprintType)
enum class ESyncedAnimationScaleMode : uint8
{
   None,
   ScaleWithSource,
   ScaleWithTarget
};

UENUM(BlueprintType)
enum class ESyncedAnimationAlignmentOrigin : uint8
{
   Center,
   Feet,
   Eyes,
   RootBone,
};

UENUM(BlueprintType)
enum class ESyncedAnimationNumberComparisonType : uint8
{
   Equal,
   LessThan,
   LessThanEqual,
   GreaterThan,
   GreaterThanEqual
};

USTRUCT(BlueprintType)
struct OSECORE_API FSyncedAnimationConfiguration
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
   FGameplayTagContainer SyncedAnimationTags;
};

UENUM(BlueprintType, meta = (Bitflags))
enum class ESyncedAnimationConstraintFlags : uint8
{
   Relax_Distance,
   Relax_Rotation,
   Relax_Side,
};

UENUM(BlueprintType)
enum class ESyncedAnimationTargetSideType : uint8
{
   Left,
   Right,
   Center
};

extern const uint32 ESyncedAnimationConstraintAllFlags;
ENUM_CLASS_FLAGS(ESyncedAnimationConstraintFlags);

namespace SyncedAnimationHelpers
{
   const USkeleton* GetSkeletonForActor(AActor* actor);
   float GetAvatarScale(const AActor* actor);
   FVector GetActorPosition(ESyncedAnimationAlignmentOrigin mode, const AActor* actor);
   float GetScaleForMode(ESyncedAnimationScaleMode mode, const AActor* source, const AActor* target);
   UAnimMontage* GetCompatibleMontage(const AActor* actor, const FGameplayTag& animationTypeTag, EESyncedAnimationRole syncRole);
   bool HasAvailableMontages(const AActor* actor, const FGameplayTag& animationTypeTag, EESyncedAnimationRole syncRole);
   const AActor* GetAvatarOrActor(const AActor* actor);

   template<class T>
   bool CompareNumberValues(ESyncedAnimationNumberComparisonType evaluationType, T lhsValue, T rhsValue)
   {
      switch(evaluationType)
      {
      case ESyncedAnimationNumberComparisonType::Equal:
         {
            return lhsValue == rhsValue;
         }
         break;
      case ESyncedAnimationNumberComparisonType::LessThan:
         {
            return lhsValue < rhsValue;
         }
         break;
      case ESyncedAnimationNumberComparisonType::LessThanEqual:
         {
            return lhsValue <= rhsValue;
         }
         break;
      case ESyncedAnimationNumberComparisonType::GreaterThan:
         {
            return lhsValue > rhsValue;
         }
         break;
      case ESyncedAnimationNumberComparisonType::GreaterThanEqual:
         {
            return lhsValue >= rhsValue;
         }
         break;
      }

      // missed a case!
      check(false);
      return false;
   }
}

USTRUCT(BlueprintType)
struct OSECORE_API FSyncedAnimationConstraintParameters
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   const AActor* Source = nullptr;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   const AActor* Target = nullptr;
   
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FGameplayTagContainer SourceAbilitySystemComponentTags;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FGameplayTagContainer TargetAbilitySystemComponentTags;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/OSECore.ESyncedAnimationConstraintFlags"))
   int ConstraintFlags = 0;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FVector SourcePosition = FVector::ZeroVector;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FRotator SourceRotation = FRotator::ZeroRotator;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FVector SourceForward = FVector::ZeroVector;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FVector SourceRight = FVector::ZeroVector;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FVector TargetPosition = FVector::ZeroVector;
   
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FRotator TargetRotation = FRotator::ZeroRotator;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FVector TargetForward = FVector::ZeroVector;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FVector TargetRight = FVector::ZeroVector;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FVector MoverPosition = FVector::ZeroVector;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FRotator MoverRotation = FRotator::ZeroRotator;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FVector MoverForward = FVector::ZeroVector;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FVector MoverRight = FVector::ZeroVector;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FVector DesiredPosition = FVector::ZeroVector;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FRotator DesiredRotation = FRotator::ZeroRotator;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FRotator MoverDeltaRotator = FRotator::ZeroRotator;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float MoverDistance = 0.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float MoverLateralDistance = 0.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float MoverHeight = 0.0f;
};

UCLASS(BlueprintType, abstract, EditInlineNew)
class OSECORE_API USyncedAnimationConstraint : public UObject
{
   GENERATED_BODY()

public:
   virtual bool CheckConstraintForUntargetedAnimation() const { return false; }
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const { return false; };
};

UCLASS(BlueprintType)
class OSECORE_API USyncedAnimationConstraintMoverRotation : public USyncedAnimationConstraint
{
   GENERATED_BODY()

public:
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FRotator MaxRotation = FRotator(180.0f, 180.0f, 180.0f);

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FRotator MinRotation = FRotator(-180.0f, -180.0f, -180.0f);
};

UCLASS(BlueprintType)
class OSECORE_API USyncedAnimationConstraintMoverDistance : public USyncedAnimationConstraint
{
   GENERATED_BODY()

public:
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float MaxDistance = 0.0f;
};

UCLASS(BlueprintType)
class OSECORE_API USyncedAnimationConstraintMoverLateralDistance : public USyncedAnimationConstraint
{
   GENERATED_BODY()

public:
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float MaxDistance = 0.0f;
};

UCLASS(BlueprintType)
class OSECORE_API USyncedAnimationConstraintMoverHeight : public USyncedAnimationConstraint
{
   GENERATED_BODY()

public:
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float MinHeight = 0.0f;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float MaxHeight = 0.0f;
};

UCLASS(BlueprintType)
class OSECORE_API USyncedAnimationConstraintSourceTags : public USyncedAnimationConstraint
{
   GENERATED_BODY()

public:
   virtual bool CheckConstraintForUntargetedAnimation() const { return true; }
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTagContainer RequiredTags;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTagContainer BlockedTags;
};

UCLASS(BlueprintType)
class OSECORE_API USyncedAnimationConstraintTargetTags : public USyncedAnimationConstraint
{
   GENERATED_BODY()

public:
   virtual bool CheckConstraintForUntargetedAnimation() const { return false; }
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTagContainer RequiredTags;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTagContainer BlockedTags;
};

UCLASS(BlueprintType)
class OSECORE_API USyncedAnimationTargetSideConstraint : public USyncedAnimationConstraint
{
   GENERATED_BODY()

public:
   virtual bool CheckConstraintForUntargetedAnimation() const { return false; }
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   ESyncedAnimationTargetSideType Side = ESyncedAnimationTargetSideType::Center;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float AngleDegrees = 0.0f;
};

UCLASS(BlueprintType)
class OSECORE_API USyncedAnimationConstraintCombatHitPath : public USyncedAnimationConstraint
{
   GENERATED_BODY()

public:
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float Radius = 20.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   EOSECombatDefenderTraceLogic DefenderTraceLogic = EOSECombatDefenderTraceLogic::TraceToCenter;
};


USTRUCT(BlueprintType)
struct OSECORE_API FSyncedAnimationAlignment
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   ESyncedAnimationAlignmentMode Mode = ESyncedAnimationAlignmentMode::TargetForward;

   // TODO: allow blend or min, etc?
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   ESyncedAnimationScaleMode AvatarScaleMode = ESyncedAnimationScaleMode::None;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
   ESyncedAnimationAlignmentOrigin SourceOrigin = ESyncedAnimationAlignmentOrigin::Center;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, AdvancedDisplay)
   ESyncedAnimationAlignmentOrigin TargetOrigin = ESyncedAnimationAlignmentOrigin::Center;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FVector RelativePosition = FVector::ZeroVector;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FRotator RelativeRotation = FRotator::ZeroRotator;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   float Duration = 0.0f;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   bool LerpToAlignment = true;
   
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   bool SweepWhenTeleportingToFinalPosition { false };
};

USTRUCT(BlueprintType)
struct OSECORE_API FSyncedAnimationCrouchState
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool ShouldBeCrouched = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool ReturnCrouchStateOnFinish = true;
};

USTRUCT(BlueprintType)
struct OSECORE_API FSyncedAnimationEntry
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool bEnabled = true;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool Debug = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag AnimationTypeTag;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float SourcePlaybackRate = 1.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   float TargetPlaybackRate = 1.0f;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool DisableInputLook = true;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool DisableInputMove = true;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool DisableCollision = true;

   // Whether the moving character should set their control rotation to match their orientation
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool SnapControlRotation = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool RequireSourceAnimation = true;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   bool IsPlayerInvulnerableForDuration = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   EPlayerCameraMode PlayerCameraMode = EPlayerCameraMode::FirstPerson;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FSyncedAnimationAlignment Alignment;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (InlineEditConditionToggle))
   bool SetSourceMovementMode = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "SetSourceMovementMode"))
   TEnumAsByte<enum EMovementMode> SourceMovementMode = EMovementMode::MOVE_None;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (InlineEditConditionToggle))
   bool SetTargetMovementMode = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "SetTargetMovementMode"))
   TEnumAsByte<enum EMovementMode> TargetMovementMode = EMovementMode::MOVE_None;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (InlineEditConditionToggle))
   bool SourceForceCrouchState = false;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta = (EditCondition = "SourceForceCrouchState"))
   FSyncedAnimationCrouchState SourceCrouchState;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FSyncedAnimationConfiguration Configuration;

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   TArray<USyncedAnimationConstraint*> Constraints;

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   TArray<UEventTimelineAction*> SourceActivateAbilityActions;

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   TArray<UEventTimelineAction*> TargetActivateAbilityActions;

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   UAbilityTask_EventTimeline* SourceTimelineTask = nullptr;

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   UAbilityTask_EventTimeline* TargetTimelineTask = nullptr;

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   TArray<UEventTimelineAction*> SourceAnimationCompleteActions;

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   TArray<UEventTimelineAction*> TargetAnimationCompleteActions;

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   TArray<UEventTimelineAction*> SourceEndAbilityActions;

   UPROPERTY(EditDefaultsOnly, Instanced, BlueprintReadOnly)
   TArray<UEventTimelineAction*> TargetEndAbilityActions;

   FString GetDebugName() const;
   bool MeetsConstraints(const FSyncedAnimationSearchParameters& parameters, bool requiresTarget) const;
};

USTRUCT(BlueprintType)
struct OSECORE_API FSyncedAnimationSearchParameters
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   const AActor* Source = nullptr;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   const AActor* Target = nullptr;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FGameplayTagContainer RequiredTags;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   FGameplayTagContainer BlockedTags;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Bitmask, BitmaskEnum = "/Script/OSECore.ESyncedAnimationConstraintFlags"))
   int ConstraintFlags = 0;

   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   int SkipPreviousAnimationIndex = INDEX_NONE;
};

UENUM(BlueprintType)
enum class EESyncedAnimationRole : uint8
{
   Source,
   Target
};

UCLASS()
class OSECORE_API USyncedAnimationFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintCallable)
   static bool SyncedAnimation_CalculateAlignment(FVector& position, FRotator& rotation, const AActor* source, const AActor* target, const FSyncedAnimationAlignment& syncedAnimationAlignment);

   UFUNCTION(BlueprintCallable, Category="Animation|Synced|OSE")
   static UAnimMontage* FindMontageForSyncedAnimation(const FSyncedAnimationEntry& entry, AActor* actor, EESyncedAnimationRole role);

   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks")
   static void ExecuteActivateAbilityActions(const FSyncedAnimationEntry& entry, AActor* actor, EESyncedAnimationRole role, AActor* otherActor);

   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks")
   static void ExecuteAnimationCompleteActions(const FSyncedAnimationEntry& entry, AActor* actor, EESyncedAnimationRole role, AActor* otherActor);

   UFUNCTION(BlueprintCallable, Category = "Ability|Tasks")
   static void ExecuteEndAbilityActions(const FSyncedAnimationEntry& entry, AActor* actor, EESyncedAnimationRole role, AActor* otherActor);
};

UCLASS(BlueprintType, Abstract, Blueprintable)
class OSECORE_API USyncedAnimationConstraintBluePrintBase : public USyncedAnimationConstraint
{
   GENERATED_BODY()

public:
   virtual bool CheckConstraintForUntargetedAnimation() const;
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;

   // Called when the game starts
   UFUNCTION(BlueprintImplementableEvent)
   bool EvaluateConstraintBlueprint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const;
   UFUNCTION(BlueprintImplementableEvent)
   bool CheckConstraintForUntargetedAnimationBlueprint() const;
};

USTRUCT(BlueprintType)
struct OSECORE_API FSyncedAnimationMontageSets
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   FGameplayTag AnimationTypeTag;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TArray<UAnimMontage*> SourceMontages;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TArray<UAnimMontage*> TargetMontages;

   const TArray<UAnimMontage*>& GetMontagesForRole(EESyncedAnimationRole role) const;
};

UCLASS(BlueprintType)
class OSECORE_API USyncedAnimationCharacterMontagesAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, meta=(TitleProperty=AnimationTypeTag))
   TArray<FSyncedAnimationMontageSets> CharacterMontageSets;

   // Get the synced animation montages for this animation type and synced animation role
   UFUNCTION(BlueprintPure)
   UAnimMontage* GetSyncedAnimationMontage(const FGameplayTag& animationTypeTag, EESyncedAnimationRole syncRole) const;
   
   // Check if there are any animation montages defined for this animation type and synced animation role
   UFUNCTION(BlueprintPure)
   bool HasAvailableSyncedAnimationMontage(const FGameplayTag& animationTypeTag, EESyncedAnimationRole syncRole) const;

#if WITH_EDITOR
   void ValidateForCharacter(const AOSECharacterBase* character, class FDataValidationContext& Context);
#endif

private:
   const FSyncedAnimationMontageSets* _GetMontageSet(const FGameplayTag& animationTypeTag) const;
};
