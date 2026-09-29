// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATSyncedAnimations.h"

// tat
#include "Character/TATCharacterAIBase.h"
#include "Combat/TATCombatFunctionLibrary.h"
#include "Developer/TATProjectSettings.h"
#include "GameFramework/GameStateBase.h"
#include "Player/TATCharacter.h"
#include "Player/TATPlayerState.h"

// ose
#include "AI/OSEAIFunctionLibrary.h"
#include "Detection/OSEDetectionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSyncedAnimations)

bool USyncedAnimationTargetAlertnessLevelConstraint::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   if (auto* character = Cast<ATATCharacterAIBase>(parameters.Target))
   {
      return SyncedAnimationHelpers::CompareNumberValues(NumberComparisonType, character->GetAlertnessLevel(), AlertnessLevel);
   }
   return false;
}


bool USyncedAnimationTakedownVulnerableConstraint::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   if (auto* targetCharacter = Cast<ATATCharacterAIBase>(parameters.Target))
   {
      // Task TOD-270 says
      //   "Target must be facing away from the player / not aware of their presence by visual or audio stim"
      // This is a reasonable (initial) approximation that is already replicated to the client (as is needed for takedown targeting)
      if(targetCharacter->GetDetectionComponent()->GetDetectionStateForPlayer(parameters.Source) >= EActorDetectionState::Identifying)
      {
         return false;
      }
      
      return true;
   }

   return false;
}

bool USyncedAnimationTeamAttitudeConstraint::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   // Use the original team for disguised actors, instead of apparent team which is the default
   EOSETeamAttitude attitude = UTATTeamAttitudeSolver::GetTeamAttitudeBetweenActorsWithDisguise(parameters.Source, parameters.Target, ETATTeamDisguiseHandling::UseOriginalTeam);

   switch (attitude)
   {
      case EOSETeamAttitude::Friendly: return AllowFriendly;
      case EOSETeamAttitude::Neutral:  return AllowNeutral;
      case EOSETeamAttitude::Hostile:  return AllowHostile;
      default:
         checkNoEntry();
         return false;
   }
}

bool USyncedAnimationValidCharacterConstraint::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters,
   const FSyncedAnimationEntry& syncedAnimation) const
{
   if (const ATATCharacter* sourceCharacter = Cast<ATATCharacter>(parameters.Source))
   {
      if (const ATATPlayerState* tatPlayerState = Cast<ATATPlayerState>(sourceCharacter->GetPlayerState()))
      {
         return ValidCharacters.Contains(tatPlayerState->GetTATCharacter());
      }
   }

   return false;
}

