// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"

// ue4
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"

#include "OSEProjectSettings.generated.h"

enum class ECustomMovementType : uint8;

class UOSESaveGame;

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[OSE] Project Settings"))
class OSECORE_API UOSEProjectSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   // for bp
   UFUNCTION(BlueprintPure, Category = "OSE Project Settings")
   static UOSEProjectSettings* GetOSEProjectSettings() { return GetMutableDefault<UOSEProjectSettings>(); }

   // for C++
   static const UOSEProjectSettings& Get() { return *GetDefault<UOSEProjectSettings>(); }

   //
   // Data-driven gameplay tags used in C++ classes
   //

   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTag ConditionBaseTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTag ConditionUnconsciousTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   TArray<FGameplayTag> ConditionTags;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTag CharacterInteractHoldTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTag CharacterInteractInstantTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTag StatusCarryingTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTag StatusCrouchingTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTag StatusSprintingTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTag StatusLedgeClimbTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTag StatusWallClimbTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTag StatusSlidingTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTag StatusFallingTag;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTagContainer LyingDownTags;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTagContainer MovementImpairingTags;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags")
   FGameplayTagContainer EffectTagsToRemoveOnTakingDamage;


   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|MovementMode")
   TMap<TEnumAsByte<EMovementMode>, FGameplayTag> MovementModeTags;
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|MovementMode")
   TMap<ECustomMovementType, FGameplayTag> CustomMovementModeTags;

   //Base Tag that all Voice Over Verb Tags should fall under.
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Voice", meta = (Categories = "VoiceVerb"))
   FGameplayTag VoiceVerbBaseTag;
   //Base Tag that all Voice Over Identity Tags should fall under.
   UPROPERTY(Config, EditDefaultsOnly, Category = "GameplayTags|Voice", meta = (Categories = "VoiceIdentity"))
   FGameplayTag VoiceIdentityBaseTag;

   UPROPERTY(Config, EditDefaultsOnly, Category = "Falling Damage")
   FOSEEffectWithSetByCallerTag FallingDamageEffect;

   UPROPERTY(Config, EditDefaultsOnly, Category = "VoiceOver")
   TSoftObjectPtr<UDataTable> VoiceOverPriority;

   UPROPERTY(Config, EditDefaultsOnly, Category = "VoiceOver")
   TSoftObjectPtr<UDataTable> VoiceIdentityInfo;

   UPROPERTY(Config, EditDefaultsOnly, Category = "VoiceOver")
   TSoftObjectPtr<UDataTable> VoiceVerbToClass;

   /// Save Game System settings
   UPROPERTY(Config, EditDefaultsOnly, Category = "SaveSystem", meta = (DisplayName = "Slot Name", Tooltip = "The game's save slot name."))
   FString SaveSlotName = TEXT("OSESAVE");

   UPROPERTY(Config, EditDefaultsOnly, Category = "SaveSystem", meta = (DisplayName = "Class", Tooltip = "The save game data class (derived from `UOSESaveGame`)."))
   TSoftClassPtr<UOSESaveGame> SaveGameClass;

   // Should we save the game on exit if the save is dirty? (always true in PIE)
   UPROPERTY(Config, EditDefaultsOnly, Category = "SaveSystem")
   bool ShouldSaveOnExit = false;

   FGameplayTag GetTagForMovementMode(EMovementMode mode, uint8 customMode) const;
};
