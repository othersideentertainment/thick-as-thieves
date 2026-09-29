// (c) 2021-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// UE4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"

// TAT
#include "TATAnimSettings.generated.h"


//--------------------------------------------------------------------------------------------------
/// Gameplay tags that map to ability animation states
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct TAT_API FTATAnimAbilityTags
{
   GENERATED_BODY()

public:

   //-----------------------------------------------------------------------------------------------
   // Condition tags
   //-----------------------------------------------------------------------------------------------

   /// Gameplay tags set when downed
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Condition, meta = (Categories = "Status,Condition"))
   FGameplayTagContainer ConditionDownedTags;

   /// Gameplay tags set when unconscious
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Condition, meta = (Categories = "Status,Condition"))
   FGameplayTagContainer ConditionUnconsciousTags;


   //-----------------------------------------------------------------------------------------------
   // Status tags
   //-----------------------------------------------------------------------------------------------

   /// Gameplay tags set when cowering
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Status, meta = (Categories = "Status,Condition"))
   FGameplayTagContainer StatusCoweringTags;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Status, meta = (Categories = "Status,Condition"))
   FGameplayTagContainer NervousTags;

   //-----------------------------------------------------------------------------------------------
   // Carry tags
   //-----------------------------------------------------------------------------------------------

   /// Gameplay tags set when being carried
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Carry, meta = (Categories = "Status,Condition"))
   FGameplayTagContainer CarriedTags;

   /// Gameplay tags set when carrying
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Carry, meta = (Categories = "Status,Condition"))
   FGameplayTagContainer CarryingTags;

   /// Gameplay tags set when being thrown
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Carry, meta = (Categories = "Status"))
   FGameplayTagContainer ThrownTags;


   //-----------------------------------------------------------------------------------------------
   // Magehand tags
   //-----------------------------------------------------------------------------------------------

   /// Gameplay tags set when casting magehand
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MageHand, meta = (Categories = "Ability.MageHand"))
   FGameplayTagContainer MageHandCastingTags;

   /// Gameplay tags set when extending magehand
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MageHand, meta = (Categories = "Ability.MageHand"))
   FGameplayTagContainer MageHandExtendedTags;

   /// Gameplay tags set when magehand is in timed interaction
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = MageHand, meta = (Categories = "Ability.MageHand"))
   FGameplayTagContainer MageHandTimedInteractionTags;


   //-----------------------------------------------------------------------------------------------
   // Wire tags
   //-----------------------------------------------------------------------------------------------

   /// Gameplay tags set when wire is attached
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wire, meta = (Categories = "Ability.Wire"))
   FGameplayTagContainer WireAttachedTags;

   /// Gameplay tags set when wire is in grappling mode
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wire, meta = (Categories = "Ability.Wire"))
   FGameplayTagContainer WireModeGrappleTags;

   /// Gameplay tags set when wire is in rappeling mode
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Wire, meta = (Categories = "Ability.Wire"))
   FGameplayTagContainer WireModeRappelTags;


   //-----------------------------------------------------------------------------------------------
   // Melee tags
   //-----------------------------------------------------------------------------------------------

   /// Gameplay tags set while in melee combat
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Melee, meta = (Categories = "Ability.Melee,Combat.Animation"))
   FGameplayTagContainer MeleeTags;

   /// Gameplay tags set while light attacking
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Melee, meta = (Categories = "Ability.Melee,Combat.Animation"))
   FGameplayTagContainer MeleeLightTags;

   /// Gameplay tags set while heavy attacking
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Melee, meta = (Categories = "Ability.Melee,Combat.Animation"))
   FGameplayTagContainer MeleeHeavyTags;

   /// Gameplay tags set while blocking (locally predicted)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Melee, meta = (Categories = "Ability.Melee.Block"))
   FGameplayTagContainer BlockLocalTags;

   /// Gameplay tags set while blocking (replicated)
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Melee, meta = (Categories = "Ability.Melee.Block"))
   FGameplayTagContainer BlockRemoteTags;

   /// Gameplay tags set while taking down overhead
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = Melee, meta = (Categories = "Combat.Attack.IsTakingDownOverhead"))
   FGameplayTagContainer TakingDownOverheadTags;
};


//--------------------------------------------------------------------------------------------------
/// TAT-specific animation settings
//--------------------------------------------------------------------------------------------------

UCLASS(Config = Game, DefaultConfig, Const, Meta = (DisplayName = "[TAT] Animation Settings"))
class TAT_API UTATAnimSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:

   static const UTATAnimSettings& Get() { return *(GetDefault<UTATAnimSettings>()); }

   UFUNCTION(BlueprintGetter)
   const FTATAnimAbilityTags& GetAbilityTags() const { return _abilityTags; }

private:

   /// Ability gameplay tags to query for animation states
   UPROPERTY(Config, EditAnywhere, Category = Abilities, BlueprintGetter = GetAbilityTags, meta = (ShowOnlyInnerProperties))
   FTATAnimAbilityTags _abilityTags;
};
