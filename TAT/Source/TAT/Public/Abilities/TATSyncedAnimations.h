// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "AI/Alertness/AlertnessEnums.h"
#include "Abilities/OSESyncedAnimations.h"

#include "TATSyncedAnimations.generated.h"

enum class ETATCharacter : uint8;

UCLASS(BlueprintType)
class TAT_API USyncedAnimationTargetAlertnessLevelConstraint : public USyncedAnimationConstraint
{
   GENERATED_BODY()

public:
   virtual bool CheckConstraintForUntargetedAnimation() const { return false; }
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   EAlertnessLevel AlertnessLevel = EAlertnessLevel::Neutral;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   ESyncedAnimationNumberComparisonType NumberComparisonType = ESyncedAnimationNumberComparisonType::Equal;
};

/// Target must be non-alert or has vulnerable-to-takedown
UCLASS(BlueprintType)
class TAT_API USyncedAnimationTakedownVulnerableConstraint : public USyncedAnimationConstraint
{
   GENERATED_BODY()

public:
   virtual bool CheckConstraintForUntargetedAnimation() const { return false; }
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;
};

/// Source must have one of the given attitudes towards Target
UCLASS(BlueprintType)
class TAT_API USyncedAnimationTeamAttitudeConstraint : public USyncedAnimationConstraint
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   bool AllowFriendly = true;

   UPROPERTY(EditDefaultsOnly)
   bool AllowNeutral = true;

   UPROPERTY(EditDefaultsOnly)
   bool AllowHostile = true;

public:
   virtual bool CheckConstraintForUntargetedAnimation() const { return false; }
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;
};

/// Source must be one of the given player characters to use this ability
UCLASS(BlueprintType)
class TAT_API USyncedAnimationValidCharacterConstraint : public USyncedAnimationConstraint
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   TArray<ETATCharacter> ValidCharacters;

public:
   virtual bool CheckConstraintForUntargetedAnimation() const { return false; }
   virtual bool EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const override;
};
