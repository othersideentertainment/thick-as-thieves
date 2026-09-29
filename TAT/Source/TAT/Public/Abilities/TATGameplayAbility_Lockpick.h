// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Lockpicking/LockpickableInterface.h"
#include "Lockpicking/TATLockpickingTypes.h"

// ose
#include "Abilities/OSEGameplayAbility.h"
#include "OSECoreCheats.h"

// ue
#include "UObject/WeakInterfacePtr.h"

#include "TATGameplayAbility_Lockpick.generated.h"

class UCurveFloat;
class UTATLockpickMinigameVariationDataAsset;

UENUM()
enum class ETATLockpickSectionRequirement : uint8
{
   RequireAll,
   RequireOne,
   RequireNone
};

UCLASS(Blueprintable)
class TAT_API UTATGameplayAbility_Lockpick : public UOSEGameplayAbility
{
   GENERATED_BODY()

   UTATGameplayAbility_Lockpick();
   
#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   // From UOSEGameplayAbility
#if OSE_CHEATS_ENABLED
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
#endif // OSE_CHEATS_ENABLED
   virtual void EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled) override;

public:
   UFUNCTION(BlueprintPure, Category = "TAT|Lockpick")
   const FTATLockpickMinigameVariation& GetLockpickMinigameVariation() const { return _lockpickMinigameVariation; }

   UFUNCTION(BlueprintPure, Category = "TAT|Lockpick")
   TScriptInterface<ILockpickableInterface> GetLockpickableActor() const;

protected:
   UFUNCTION(BlueprintCallable, Category = "TAT|Lockpick")
   void InitFromLockpickable(TScriptInterface<ILockpickableInterface> lockpickableActor, const FTATLockpickMinigameVariation& lockpickMinigameVariation);

   UFUNCTION(BlueprintCallable, Category = "TAT|Lockpick")
   void TryStartNewTrack();

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT|Lockpick")
   void OnNewTrackStarted();

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT|Lockpick")
   void OnTrackProgressChanged(int32 trackIndex, float trackProgress);

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT|Lockpick")
   void OnTrackInteractionTriggered(int32 trackIndex, float trackProgress);

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT|Lockpick")
   void OnTrackReachedEnd(int32 trackIndex, bool completedRequiredSections);

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT|Lockpick")
   void OnAllTracksReachedEnd();

   UFUNCTION(BlueprintCallable, Category = "TAT|Lockpick")
   void TickTrackProgress(float deltaTime);

   UFUNCTION(BlueprintCallable, Category = "TAT|Lockpick")
   void TriggerTrackInteract();

   UFUNCTION(BlueprintCallable, Category = "TAT|Lockpick")
   void MarkTrackCompleted();

   UFUNCTION(BlueprintCallable, Category = "TAT|Lockpick")
   int32 GetCurrentTrack() const;

   // Tracks the current progress made within the current lockpick ring track
   UPROPERTY(Transient, BlueprintReadOnly)
   float CurrentTrackTimeSeconds = 0.f;

   // Data asset storing each possible lockpick minigame variation
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Lockpick")
   UTATLockpickMinigameVariationDataAsset* LockpickMinigameVariationDataAsset = nullptr;

   // Type of sections that must be interacted with to complete a track
   UPROPERTY(EditDefaultsOnly, Category = "TAT|Lockpick", meta = (Categories = "Lockpick.TrackSection"))
   FGameplayTag RequiredSectionTag;

   UPROPERTY(EditDefaultsOnly, Category = "TAT|Lockpick")
   ETATLockpickSectionRequirement SectionRequirement = ETATLockpickSectionRequirement::RequireOne;

private:
   // Selected lockpick minigame variation
   UPROPERTY(Transient)
   FTATLockpickMinigameVariation _lockpickMinigameVariation;

   // Lockable actor being picked
   TWeakInterfacePtr<ILockpickableInterface> _lockpickableActor;

   // Which lockpick ring are we currently progressions across?
   int32 _currentTrackIndex = INDEX_NONE;

   // All the track section definitions that make up the current lockpick ring
   FTATLockpickTrackLayerManager _currentTrackLayerStack;

   // Cache of the last gameplay ability triggered by the current section
   UPROPERTY(Transient)
   FGameplayTag _lastTriggeredAbility = FGameplayTag::EmptyTag;

   bool _AreRequiredSectionsComplete() const;

   // Returns the track speed of the currently active track section
   float _GetTrackSpeed() const;

   // Executes the gameplay cue and ability (if any) of the currently active track section
   void _TriggerCurrentSectionEvents();

   UFUNCTION()
   void _OnLockpickableActorEndPlay(AActor* actor, EEndPlayReason::Type endPlayReason);

   UFUNCTION()
   void _OnLockpickableActorCancelledLockpicking();
};
