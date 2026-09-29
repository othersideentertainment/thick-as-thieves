// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"

// OSE
#include "Animation/Graph/OSEAnimData.h"

// TAT
#include "TATAnimData.generated.h"

// Forward references
class IGameplayTagAssetInterface;
struct FTATAnimAbilityTags;


//--------------------------------------------------------------------------------------------------
/// Ability info populated from gameplay tags.
/// The tags are specified in project settings.
/// \see FTATAnimAbilityTags
/// \see UTATAnimSettings
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct TAT_API FTATAnimAbilityData: public FOSEBaseAnimData
{
   GENERATED_BODY()

public:

   FTATAnimAbilityData();

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Condition) uint8 IsDowned : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Condition) uint8 IsUnconscious : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Status) uint8 IsCowering : 1;
   
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Carry) uint8 IsBeingCarried : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Carry) uint8 IsCarrying : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Carry) uint8 IsBeingThrown : 1;
   
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MageHand) uint8 IsMageHandCasting : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MageHand) uint8 IsMageHandExtended : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MageHand) uint8 IsMageHandInTimedInteraction : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wire) uint8 IsWireAttached : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wire) uint8 IsWireInGrappleMode : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wire) uint8 IsWireInRappelMode : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat) uint8 IsInMelee : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat) uint8 IsInLightMelee : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat) uint8 IsInHeavyMelee : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat) uint8 IsBlocking : 1;
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat) uint8 IsTakingDownOverhead : 1;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Combat) uint8 IsNervous : 1;

   
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Control) uint8 IsLocalPlayer : 1;


public:

   /// Updates ability info given tag containers and an interface
   void Update(const IGameplayTagAssetInterface* tagInterface, const FTATAnimAbilityTags& abilityTags, bool isLocallyControlled);
   virtual void Update(const FOSEAnimActorInfo& actorInfo) override;
};

