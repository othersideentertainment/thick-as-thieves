// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEGameplayAbility.h"

// ue5
#include "Engine/DataAsset.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "GameplayTagContainer.h"

#include "OSEAbilityInputBinds.generated.h"

class UInputAction;
class UOSEAbilitySystemComponent;

//--------------------------------------------------------------------------------------------------
/// Default enumeration mapping input actions that can trigger abilities.
/// The name of the enum values MUST MATCH the input action defined in DefaultInput.ini to work
// Note: You'll have to restart your editor for this list to update the editor
//--------------------------------------------------------------------------------------------------

UENUM(BlueprintType)
enum class EAbilityInputType : uint8
{
   AbilityActionPrimary        UMETA(DisplayName = "AbilityActionPrimary"),
   AbilityActionPrimaryTap     UMETA(DisplayName = "AbilityActionPrimaryTap"),
   AbilityActionPrimaryHold    UMETA(DisplayName = "AbilityActionPrimaryHold"),
   AbilityActionSecondary      UMETA(DisplayName = "AbilityActionSecondary"),
   AbilityMovementJump         UMETA(DisplayName = "AbilityMovementJump"),
   AbilityMovementCrouch       UMETA(DisplayName = "AbilityMovementCrouch"),
   AbilityMovementDodge        UMETA(DisplayName = "AbilityMovementDodge"),
   AbilityInteract             UMETA(DisplayName = "AbilityInteract"),
   AbilityMovementCrouchHold   UMETA(DisplayName = "AbilityMovementCrouchHold"),
   AbilityMeleeToggle          UMETA(DisplayName = "AbilityMeleeToggle"),
   AbilityPingSystem           UMETA(DisplayName = "AbilityPingSystem"),
   AbilitySelfRevive           UMETA(DisplayName = "AbilitySelfRevive"),
   AbilitySelfReviveTeam       UMETA(DisplayName = "AbilitySelfReviveTeam"),
   AbilityEquipNextItem        UMETA(DisplayName = "AbilityEquipNextTool"),
   AbilityEquipPrevItem        UMETA(DisplayName = "AbilityEquipPrevTool"),
   AbilityAttack               UMETA(DisplayName = "AbilityAttack"),
   AbilityBlock                UMETA(DisplayName = "AbilityBlock"),
   AbilityRangedAttack         UMETA(DisplayName = "AbilityRangedAttack"),
   AbilityRangedReload         UMETA(DisplayName = "AbilityRangedReload"),
   AbilitySpecialAttack        UMETA(DisplayName = "AbilitySpecialAttack"),
   Ability_01                  UMETA(DisplayName = "Ability_01"),
   Ability_02                  UMETA(DisplayName = "Ability_02"),
   Ability_03                  UMETA(DisplayName = "Ability_03"),
   Ability_04                  UMETA(DisplayName = "Ability_04"),
   Ability_05                  UMETA(DisplayName = "Ability_05"),
   Ability_06                  UMETA(DisplayName = "Ability_06"),
   Ability_07                  UMETA(DisplayName = "Ability_07"),
   Ability_08                  UMETA(DisplayName = "Ability_08"),
   Ability_09                  UMETA(DisplayName = "Ability_09"),
   Ability_10                  UMETA(DisplayName = "Ability_10"),
   Ability_11                  UMETA(DisplayName = "Ability_11"),
   Ability_12                  UMETA(DisplayName = "Ability_12"),
   Ability_13                  UMETA(DisplayName = "Ability_13"),
   Ability_14                  UMETA(DisplayName = "Ability_14"),
   Ability_15                  UMETA(DisplayName = "Ability_15"),
   Ability_16                  UMETA(DisplayName = "Ability_16"),
   Ability_17                  UMETA(DisplayName = "Ability_17"),
   Ability_18                  UMETA(DisplayName = "Ability_18"),
   Ability_19                  UMETA(DisplayName = "Ability_19"),
   Ability_20                  UMETA(DisplayName = "Ability_20"),   
   AbilityCancel               UMETA(DisplayName = "AbilityCancel"),
   AbilityDraw                 UMETA(DisplayName = "AbilityDraw"),
   AbilityQuickAccess          UMETA(DisplayName = "AbilityQuickAccess"),

   // None/Invalid
   None                        UMETA(DisplayName = "None"),
};

UCLASS()
class OSECORE_API UOSEAbilityInputFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:

   /// Ability-specific input lookup
   UFUNCTION(BlueprintPure, Category = "Input")
   static UInputAction* GetInputActionFromAbilityType(AActor* character, EAbilityInputType inputType);
   /// Ability-specific input lookup
   UFUNCTION(BlueprintPure, Category = "Input")
   static FKey GetKeyForAbilityInput(AActor* character, EAbilityInputType inputType, EOSEInputHardwareType inputHardwareType);
   /// Ability-specific input lookup
   UFUNCTION(BlueprintPure, Category = "Input")
   static FText GetKeyTextForAbilityInput(AActor* character, EAbilityInputType inputType, EOSEInputHardwareType inputHardwareType, bool longDisplayName = false);
};

//--------------------------------------------------------------------------------------------------
/// Associates an input enumeration value to a gameplay ability class.
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAbilityBindRequirements
{
   GENERATED_BODY()

   /// The upgrade tag required to grant the ability (or none)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Requirements")
   FGameplayTag RequiredUpgradeTag;

   /// The level of the upgrade required to grant the ability
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Requirements")
   int32 RequiredUpgradeLevel = 1;

   bool IsMet(const UOSEAbilitySystemComponent* asc) const;
};

USTRUCT(BlueprintType)
struct OSECORE_API FOSEAbilityBindInfo
{
   GENERATED_BODY()

public:

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (InlineEditConditionToggle))
   bool bBindToInput = false;

   /// The input enumeration to map to the activation of the ability
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability", meta = (EditCondition = "bBindToInput"))
   EAbilityInputType InputCommand = EAbilityInputType::None;

   /// The actual ability class
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
   TSubclassOf<class UOSEGameplayAbility> AbilityClass;

   /// The upgrade tag required to grant the ability (or none)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Ability")
   FOSEAbilityBindRequirements Requirements;
};


//--------------------------------------------------------------------------------------------------
/// Wrapper class that returns a gameplay input binding for the specified template enum type
//--------------------------------------------------------------------------------------------------

template <typename EnumType>
struct OSECORE_API FOSEAbilityInputBinds
{
public:

   static FGameplayAbilityInputBinds Get()
   {
      // Our ASC handles calling the base class confirm/cancel
      // via enhanced input, so we're no longer using these.
      static const FString sAbilityConfirmName(TEXT(""));
      static const FString sAbilityCancelName(TEXT(""));

      return FGameplayAbilityInputBinds(
         sAbilityConfirmName,                  // Defines command string that will be bound to Confirm Targeting
         sAbilityCancelName,                   // Defines command string that will be bound to Cancel Targeting
         FTopLevelAssetPath(StaticEnum<EnumType>())     // Returns enum to use for ability binds. E.g., "Ability1"-"Ability9" input commands will be bound to ability activations inside the AbiltiySystemComponent
      );
   }
};

//--------------------------------------------------------------------------------------------------
/// A data asset that maps EAbilityInputType enums to Enhanced Input Actions
//--------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class OSECORE_API UEnhancedAbilityInputActionsAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UEnhancedAbilityInputActionsAsset();

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, EditFixedSize, Category = "Input|Enhanced")
   TMap<EAbilityInputType, UInputAction*> AbilityInputToEnhancedInputActionMapping;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
   UInputAction* ConfirmInputAction = nullptr;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Input|Enhanced")
   UInputAction* CancelInputAction = nullptr;

   bool FindAbilityInputFromInputAction(const UInputAction* searchInputAction, EAbilityInputType& inputType) const;
   UInputAction* FindInputActionFromAbilityType(EAbilityInputType inputType) const;
};

