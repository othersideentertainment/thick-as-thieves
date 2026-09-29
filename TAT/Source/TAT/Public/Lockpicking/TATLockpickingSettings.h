// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "GameFramework/TATDifficulty.h"
#include "Lockpicking/TATLockpickingTypes.h"

// ose
#include "Abilities/OSEHeldActionCues.h"

// ue
#include "GameplayTagContainer.h"
#include "Engine/DeveloperSettings.h"
#include "UObject/SoftObjectPtr.h"

#include "TATLockpickingSettings.generated.h"

USTRUCT(BlueprintType)
struct TAT_API FTATLockInteractPrompts
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   FText LockpickPrompt;

   // With {LockLevel} replacement
   UPROPERTY(EditDefaultsOnly)
   FText LockLevelPrompt;

   UPROPERTY(EditDefaultsOnly)
   FText UnlockWithKeyPrompt;

   UPROPERTY(EditDefaultsOnly)
   FText LockWithKeyPrompt;

   UPROPERTY(EditDefaultsOnly)
   FText LockWithoutKeyPrompt;
   
   // Prompt for when the object can't be lockpicked but can be unlocked with a key
   UPROPERTY(EditDefaultsOnly)
   FText UnlockRequiresKeyPrompt;

   // Prompt for when a player who can't lockpick (e.g. astral projection) is looking at a locked object that CAN'T be unlocked from another side.
   UPROPERTY(EditDefaultsOnly)
   FText CannotLockpickPrompt;

   // Prompt for when a player who can't lockpick (e.g. astral projection) is looking at a locked object that CAN be unlocked from another side.
   UPROPERTY(EditDefaultsOnly)
   FText CannotLockpickWrongSidePrompt;
};

USTRUCT(BlueprintType)
struct TAT_API FTATLockpickTrackSectionTypeConfig
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere)
   float TrackSpeed = 1.0f;

   UPROPERTY(EditAnywhere, meta = (Categories = "Lockpick.TrackSection"))
   FGameplayTag InteractedSectionType = FGameplayTag::EmptyTag;

   UPROPERTY(EditAnywhere, meta = (UIMin = "0", ClampMin = "0"))
   float InteractedSectionDurationSeconds = 0.0f;

   // Activated when sections of this type first become active, cancelled when they are no longer active
   UPROPERTY(EditAnywhere, meta = (Categories = "Ability"))
   FGameplayTag GameplayAbilityTag = FGameplayTag::EmptyTag;
};

UCLASS(Config = Game, DefaultConfig, Meta = (DisplayName = "[TAT] Lockpicking Settings"))
class TAT_API UTATLockpickingSettings : public UDeveloperSettings
{
   GENERATED_BODY()

public:
   UTATLockpickingSettings();

   UFUNCTION(BlueprintPure, Category = "TAT|Lockpicking")
   static UTATLockpickingSettings* GetLockpickingSettings() { return GetMutableDefault<UTATLockpickingSettings>(); }

   // C++ access
   static const UTATLockpickingSettings& GetLockpickingSettingsRef() { return *GetDefault<UTATLockpickingSettings>(); }

   // Offset to the lock level at a given difficulty level
   UPROPERTY(Config, EditAnywhere, Category = "Difficulty", meta = (ArraySizeEnum="/Script/TAT.ETATDifficulty"))
   int32 DifficultyLevelOffset[static_cast<int>(ETATDifficulty::MAX)] = {-1, 0, 1 };

   // Stat to be incremented when the player picks a lock
   UPROPERTY(Config, EditAnywhere, Category = "Lockpicking", meta = (Categories = "PlayerStats"))
   FGameplayTag LocksPickedPlayerStat;

   UPROPERTY(Config, EditAnywhere, Category = "Lockpicking", meta = (Categories = "GameplayCue"))
   FGameplayTag UnlockWithLockpickingCue;

   // Key Stuff (should this be here, since not lockpicking as such, just locks?)

   /// Number of seconds it takes to use a key to lock or unlock something
   UPROPERTY(Config, EditAnywhere, Category = "Keys")
   float UseKeyInteractTime = 0.5f;

   UPROPERTY(Config, EditAnywhere, Category = "Keys", meta = (Categories = "GameplayCue"))
   FGameplayTag UnlockWithKeyCue;

   UPROPERTY(Config, EditAnywhere, Category = "Keys", meta = (Categories = "GameplayCue"))
   FGameplayTag LockWithKeyCue;

   UPROPERTY(Config, EditAnywhere, Category = "Keys", meta = (Categories = "InteractAnimation.Hold"))
   FGameplayTag LockpickInteractAnimation;

   UPROPERTY(Config, EditAnywhere, Category = "Keys", meta = (Categories = "InteractAnimation.Hold"))
   FGameplayTag UseKeyInteractAnimation;
   
   UPROPERTY(Config, EditAnywhere, Category = "Prompts")
   FTATLockInteractPrompts InteractPrompts;

   UPROPERTY(Config, EditAnywhere, Category = "Combination Lock")
   FText CombinationLockPrompt;

   UPROPERTY(Config, EditAnywhere, Category = "Combination Lock")
   FGameplayTag CombinationLockEvent;

   // interacting time when re-locking an interactable
   UPROPERTY(Config, EditAnywhere, Category = "Explicit Locking")
   float LockWithoutKeyInteractTime = 2.0f;

   UPROPERTY(Config, EditAnywhere, Category = "Explicit Locking")
   FOSEHeldActionCues LockWithoutKeyHeldCues;
   
   UPROPERTY(Config, EditAnywhere, Category = "Explicit Locking", meta = (Categories = "GameplayCue"))
   FGameplayTag LockWithoutKeyCue;

   UPROPERTY(Config, EditAnywhere, Category = "Explicit Locking", meta = (Categories = "InteractAnimation.Hold"))
   FGameplayTag LockWithoutKeyInteractAnimation;

   UPROPERTY(Config, EditAnywhere, Category = "Lockpicking")
   FOSEHeldActionCues LockpickingHeldActionCues;

   // Defines all track sections available for lockpicking minigame variations, and the data pertaining to them.
   UPROPERTY(Config, EditAnywhere, Category = "Lockpicking", meta = (Categories = "Lockpick.TrackSection"))
   TMap<FGameplayTag, FTATLockpickTrackSectionTypeConfig> TrackSectionConfigs;

   // Default track section type tag for UTATLockpickTrackWidget to fill any track ring to 360.0f.
   UPROPERTY(Config, EditAnywhere, Category = "Lockpicking", meta = (Categories = "Lockpick.TrackSection"))
   FGameplayTag EmptyTrackSectionType;

   // Default track section type tag for FTATLockpickMinigameTrackDefinition::DefaultSectionType.
   UPROPERTY(Config, EditAnywhere, Category = "Lockpicking", meta = (Categories = "Lockpick.TrackSection"))
   FGameplayTag DefaultTrackSectionType;

   // Default track section type tag for FTATLockpickTrackSectionDefinition::SectionType.
   UPROPERTY(Config, EditAnywhere, Category = "Lockpicking", meta = (Categories = "Lockpick.TrackSection"))
   FGameplayTag InitialDefinedTrackSectionType;

   // Should we delete track widgets after finishing them and switching to another?
   UPROPERTY(Config, EditAnywhere, Category = "Lockpicking")
   bool DestroyTracksOnFinish = true;
};
