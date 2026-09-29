// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Abilities/GameplayAbilityTargetTypes.h"

#include "TATGameplayAbilityTargetData_Dodge.generated.h"

class UOSECharacterMovement;

USTRUCT(BlueprintType)
struct TAT_API FTATGameplayAbilityTargetData_Dodge : public FGameplayAbilityTargetData
{
   GENERATED_BODY()

   FTATGameplayAbilityTargetData_Dodge() {}
   FTATGameplayAbilityTargetData_Dodge(const FVector& dodgeEndLocation, const FVector& dodgeDirection, bool isBackstep) 
      : DodgeEndLocation(dodgeEndLocation), DodgeDirection(dodgeDirection), IsBackstep(isBackstep){}

   // From FGameplayAbilityTargetData
   virtual UScriptStruct* GetScriptStruct() const override
   {
      return FTATGameplayAbilityTargetData_Dodge::StaticStruct();
   }

   virtual FString ToString() const override
   {
      return TEXT("FTATGameplayAbilityTargetData_Dodge");
   }
   
   virtual FVector GetEndPoint() const override
   {
      return DodgeEndLocation;
   }

   bool NetSerialize(FArchive& ar, class UPackageMap* map, bool& outSuccess);

   // Returns target data generated using the character's acceleration (i.e. movement input)
   static FTATGameplayAbilityTargetData_Dodge* MakeFromCharacterMovement(const UOSECharacterMovement* characterMovement);

   // Returns target data for character generated from provided dodge vector. Caller must specify if performing a backstep
   // NOTE: assumes dodgeVector was generated via UOSECharacterMovement::GetDodgeDirection() / MakeDodgeDirectionFromVector()
   static FTATGameplayAbilityTargetData_Dodge* MakeFromDodgeDirection(const ACharacter* character, FVector dodgeVector, bool isBackstep);

public:
   UPROPERTY(BlueprintReadOnly)
   FVector DodgeEndLocation = FVector::ZeroVector;

   // Direction relative to player's facing direction
   UPROPERTY(BlueprintReadOnly)
   FVector DodgeDirection = FVector::ZeroVector;

   // True if dodge was performed while stationary
   UPROPERTY(BlueprintReadOnly)
   uint8 IsBackstep : 1;
};

template<>
struct TStructOpsTypeTraits<FTATGameplayAbilityTargetData_Dodge> : public TStructOpsTypeTraitsBase2<FTATGameplayAbilityTargetData_Dodge>
{
   enum
   {
      WithNetSerializer = true // For now this is REQUIRED for FGameplayAbilityTargetDataHandle net serialization to work
   };
};
