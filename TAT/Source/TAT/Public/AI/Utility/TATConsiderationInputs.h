// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Alertness/DetectionEnums.h"
#include "AI/Utility/ConsiderationInput.h"
#include "Character/OSETeamInterface.h"
#include "Math/OSEMathFunctionLibrary.h"

// ue4
#include "CoreMinimal.h"
#include "Engine/DataTable.h"

// self
#include "TATConsiderationInputs.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogTATConsiderationInputs, Log, All);

enum class EAlarmState : uint8;
enum class EKnowledgeSource : uint8;

namespace TATConsiderationFixup
{
   TAT_API UConsiderationInput* FixupConsideration(UConsiderationInput* input, UObject* outerAsset);
}


UCLASS()
class TAT_API UConsiderationInput_IdentificationValueMatches : public UConsiderationInput
{
   GENERATED_BODY()
public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod { EOSEComparisonMethod::EqualTo };

   UPROPERTY(EditDefaultsOnly)
   float RequiredValue { 0.f };
};

UCLASS()
class TAT_API UConsiderationInput_CanSeeTarget : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

};

UCLASS()
class TAT_API UConsiderationInput_MyDetectionStateForTarget : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EActorDetectionState State = EActorDetectionState::Observing;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::EqualTo;
};

UCLASS()
class TAT_API UConsiderationInput_MyDetectionForTargetIsNonZero : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_ShouldInvestigateTarget : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_IsInvestigatingTarget : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_MyAlertnessIsNeutral : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

};

UCLASS()
class TAT_API UConsiderationInput_MyAlertnessIsExactly : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EAlertnessLevel Level = EAlertnessLevel::Neutral;
};

UCLASS()
class TAT_API UConsiderationInput_MyAlertnessIsGreaterThanOrEqualTo : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EAlertnessLevel Level = EAlertnessLevel::Neutral;
};

UCLASS()
class TAT_API UConsiderationInput_MyAlertnessValue : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EAlertnessLevel Level = EAlertnessLevel::Neutral;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::EqualTo;
};


UCLASS()
class TAT_API UConsiderationInput_TargetAlertnessValue : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EAlertnessLevel Level = EAlertnessLevel::Neutral;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::EqualTo;
};




UCLASS()
class TAT_API UConsiderationInput_HasPathToLastKnownTargetLocation : public UConsiderationInput
{
   GENERATED_BODY()
 
public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Distance")
   bool bUsePathFinding { false };
};

UCLASS(Abstract)
class TAT_API UConsiderationInput_NavMeshDistanceToLocationBase : public UConsiderationInput
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly,Category="TAT|Distance", meta=(Units="cm"))
   float Distance = 100.0f;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;

protected:
   virtual float _InternalGetValue(const AActor* const fromActor, FVector targetLocation) const;
};

UCLASS()
class TAT_API UConsiderationInput_NavMeshDistanceToLastKnownLocationComparison : public UConsiderationInput_NavMeshDistanceToLocationBase
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_HasFullPathToTargetLocation : public UConsiderationInput
{
   GENERATED_BODY()
 
public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly, Category="TAT|Distance")
   bool bUsePathFinding { false };
};

UCLASS()
class TAT_API UConsiderationInput_IsThisMyProblematicTarget : public UConsiderationInput
{
   GENERATED_BODY()
 
public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_IsTargetACharacter : public UConsiderationInput
{
   GENERATED_BODY()
 
public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_IsTargetNPC : public UConsiderationInput
{
   GENERATED_BODY()
 
public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_IsActorBroken : public UConsiderationInput
{
   GENERATED_BODY()
 
public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_DoIHaveAProblematicTarget : public UConsiderationInput
{
   GENERATED_BODY()
 
public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_CanIUseMagicKnowledgeToFindHelperAlly : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_DoIHaveAHelperAlly : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};


UCLASS()
class TAT_API UConsiderationInput_IsThisMyHelperAlly : public UConsiderationInput
{
   GENERATED_BODY()
 
public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_AmIAHelperAlly : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_HasProblematicTargetBeenDestroyed : public UConsiderationInput
{
   GENERATED_BODY()
 
public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_DoINeedHelp : public UConsiderationInput
{
   GENERATED_BODY()
 
public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_DoIAlreadyHaveHelp : public UConsiderationInput
{
   GENERATED_BODY()
 
public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_TimeSinceLastAttackedByTarget : public UConsiderationInput
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
class TAT_API UConsiderationInput_KnowledgeSource : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EKnowledgeSource KnowledgeSource;
};

UCLASS()
class TAT_API UConsiderationInput_TimeSinceTargetLastSeen : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Min = 0.0f;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float Max = 0.0f;

   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   bool RequiresValidLastKnownLocation = true;
};

UCLASS()
class TAT_API UConsiderationInput_HasLineOfSightToTarget : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_NavMeshDistanceToTargetNearbyActor : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_CachedNavMeshDistanceToTarget : public UConsiderationInput
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
class TAT_API UConsiderationInput_CachedNavMeshDistanceToTargetComparison : public UConsiderationInput
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
class TAT_API UConsiderationInput_FakeCyclicalIntelligence : public UConsiderationInput
{
   GENERATED_BODY()

public:

   // time to get from 1 to -1 then back to 1
   UPROPERTY(EditDefaultsOnly)
   float SecondsPerCycle = 2.0f;

   // within our SecondsPerCycle how often will this be a non-zero value?
   UPROPERTY(Transient, EditDefaultsOnly, meta = (EditCondition = "false"))
   float SecondsUpTime = 0.0f;

   // within our SecondsPerCycle how often will this be a zero value?
   UPROPERTY(Transient, EditDefaultsOnly, meta = (EditCondition = "false"))
   float SecondsDownTime = 0.0f;

   // value between -1 and 1 which defines what part of our 
   UPROPERTY(EditDefaultsOnly)
   float MinSmartness = 0.7f;

   UPROPERTY(EditDefaultsOnly)
   float MaxSmartness = 0.9f;

   virtual float GetValue(const FConsiderationContext& ctx) const override;

protected:
#if WITH_EDITOR
   // from UObject
   virtual void PostLoad() override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostEditUndo() override;
#endif // WITH_EDITOR

private:
   void _ClampValuesInRange();
   void _RefreshUptime();
};

UCLASS()
class TAT_API UConsiderationInput_AICombatCombo : public UConsiderationInput
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, meta = (Categories = "Ability"))
   FGameplayTag AbilityTag;

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_TargetAllyIsBehavingSuspiciously : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_TargetIsInIdentificationState : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

protected:
   UPROPERTY(EditDefaultsOnly)
   EActorDetectionState RequiredState { EActorDetectionState::Identified };
};

UCLASS()
class TAT_API UConsiderationInput_TargetIsIdentified : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_HasAnyIdentifiedTarget : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EOSETeamAttitude Attitude = EOSETeamAttitude::Hostile;
};

UCLASS()
class TAT_API UConsiderationInput_HasAnyForcedStateTags : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};
UCLASS()
class TAT_API UConsiderationInput_HasBeenRequestedToMoveFromLocation : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_IsStimFromKnownPlayer : public UConsiderationInput_StimConsiderationBase
{
   GENERATED_BODY()

protected:
   virtual float _GetStimValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_StimHasTag : public UConsiderationInput_StimConsiderationBase
{
   GENERATED_BODY()
   
protected:
   virtual float _GetStimValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly, meta=(RowType="/Script/OSEAI.OSEStimSettings"))
   FDataTableRowHandle StimInfoRow;
};

UCLASS()
class TAT_API UConsiderationInput_IsLocationWithinGuardedPrivateArea : public UConsiderationInput
{
   GENERATED_BODY()
   
public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};


UCLASS()
class TAT_API UConsiderationInput_StimInstigatorTeamAttitudeComparison : public UConsiderationInput_StimConsiderationBase
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   EOSETeamAttitude Attitude = EOSETeamAttitude::Hostile;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::EqualTo;

protected:
   virtual float _GetStimValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_TargetAttitudeComparison : public UConsiderationInput
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   EOSETeamAttitude Attitude = EOSETeamAttitude::Hostile;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::EqualTo;

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_HasTrait : public UConsiderationInput
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, meta = (Categories = "AI.Trait"))
   FGameplayTag Trait;

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_TargetHasTrait : public UConsiderationInput
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, meta = (Categories = "AI.Trait"))
   FGameplayTag Trait;

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_DistanceFromGuardPositionComparision : public UConsiderationInput
{
   GENERATED_BODY()
public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly, Category="TAT|Distance")
   bool bUsePathfinding { true };
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Distance", meta=(Units="cm"))
   float Distance = 100.0f;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::LessThanOrEqualTo;
};

UCLASS()
class TAT_API UConsiderationInput_SmartObjectIsWithinRange : public UConsiderationInput_NavMeshDistanceToLocationBase
{
   GENERATED_BODY()
public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_SmartObjectHasTag : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   FGameplayTag GameplayTag;
};

UCLASS()
class TAT_API UConsiderationInput_AlertnessValueChangedFromBlackboardValue : public UConsiderationInput
{
   GENERATED_BODY()
public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_HasMajorLoot : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class TAT_API UConsiderationInput_AlarmStationState : public UConsiderationInput
{
   GENERATED_BODY()

public:

   virtual float GetValue(const FConsiderationContext& ctx) const override;

   UPROPERTY(EditDefaultsOnly)
   EAlarmState AlarmState;

   UPROPERTY(EditDefaultsOnly)
   EOSEComparisonMethod ComparisonMethod = EOSEComparisonMethod::EqualTo;
};

UCLASS()
class UConsiderationInput_LivingWorldAgentHasClaimedHandles : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class UConsiderationInput_GetDistanceOfClosestActorOfKnowledgeTypeInRange : public UConsiderationInput
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, Category="TAT|Distance")
   EOSETeamAttitude RequiredAttitude { EOSETeamAttitude::Neutral };

   UPROPERTY(EditDefaultsOnly,Category="TAT|Distance", meta=(Units="cm"))
   float MinDistanceForKnowledgeActor = 100.0f;
   
public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

UCLASS()
class UConsiderationInput_HasIndividualKnowledgeOfTarget : public UConsiderationInput
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Category="TAT|Knowledge")
   FGameplayTagQuery QueryToRun;
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

// Returns full score if the AI has already registered for a reaction role
// against the consideration context target.
UCLASS()
class UConsiderationInput_HasAnyReactionRoleClaimed : public UConsiderationInput
{
   GENERATED_BODY()

public:
   virtual float GetValue(const FConsiderationContext& ctx) const override;
};

// Returns full score if the AI has already registered for the specified reaction
// role or if there is an available registrant slot for the role.
UCLASS()
class UConsiderationInput_IsReactionRoleAvailableOrClaimed : public UConsiderationInput
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Reactions", meta = (Categories = "AI.Behavior.Role"))
   FGameplayTag RoleTag;

   // If set to true, if no event configuration exists for this utility target,
   // this consideration input will succeed anyways. 
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Reactions")
   bool SucceedIfNoEventConfig = false;

   virtual float GetValue(const FConsiderationContext& ctx) const override;
};
