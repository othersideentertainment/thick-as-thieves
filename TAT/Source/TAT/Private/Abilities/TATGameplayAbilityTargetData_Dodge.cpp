// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATGameplayAbilityTargetData_Dodge.h"

// tat
#include "Combat/TATCombatSettings.h"

// ose
#include "Character/OSECharacterMovement.h"

// ue
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbilityTargetData_Dodge)

bool FTATGameplayAbilityTargetData_Dodge::NetSerialize(FArchive& ar, UPackageMap* map, bool& outSuccess)
{
   DodgeEndLocation.NetSerialize(ar, map, outSuccess);
   DodgeDirection.NetSerialize(ar, map, outSuccess);

   // Probably won't extend with more bits, but may as well make it easy should we change our mind
   uint8 flags = IsBackstep;
   ar.SerializeBits(&flags, 1);

   outSuccess = true;
   return true;
}

FTATGameplayAbilityTargetData_Dodge* FTATGameplayAbilityTargetData_Dodge::MakeFromCharacterMovement(const UOSECharacterMovement* characterMovement)
{
   check(characterMovement);

   const ACharacter* character = characterMovement->GetCharacterOwner();
   check(character);

   // Scale dodge direction by plugging acceleration magnitude into curve
   bool isStationaryDodge = false;
   const FVector dodgeVector = characterMovement->GetDodgeDirection(isStationaryDodge);

   // ASSUMPTION: stationary dodge = backstep
   const bool isBackstep = isStationaryDodge;
   return MakeFromDodgeDirection(character, dodgeVector, isBackstep);
}

FTATGameplayAbilityTargetData_Dodge* FTATGameplayAbilityTargetData_Dodge::MakeFromDodgeDirection(const ACharacter* character, FVector dodgeVector, bool isBackstep)
{
   const UTATCombatSettings& combatSettings = UTATCombatSettings::Get();
   const float dodgeMagnitude = isBackstep ? combatSettings.BackstepDodgeMagnitude : combatSettings.MovingDodgeMagnitude;
   dodgeVector *= dodgeMagnitude;

   // Pass dodge end location/rotation in event payload
   // NOTE: client-provided end location is validated by triggered ability
   const FVector dodgeEndLocation = dodgeVector + character->GetActorLocation();
   const FVector dodgeDirectionRelative = character->GetTransform().InverseTransformVectorNoScale(dodgeVector);

   /** Note: These are cleaned up by the FGameplayAbilityTargetDataHandle (via an internal TSharedPtr) */
   return new FTATGameplayAbilityTargetData_Dodge(dodgeEndLocation, dodgeDirectionRelative, isBackstep);
}
