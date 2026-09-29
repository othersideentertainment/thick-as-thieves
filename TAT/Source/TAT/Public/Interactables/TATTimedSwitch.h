// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATInteractableToggle.h"
#include "Interactables/TATParameterizedTextCache.h"

// ue
#include "CoreMinimal.h"
#include "StructUtils/InstancedStruct.h"

#include "TATTimedSwitch.generated.h"

struct FTATToggleResolver;

UENUM()
enum class ETATTimedSwitchBehavior : uint8
{
   TurnOnTarget,
   TurnOffTarget
};

// A switch that sets the state on a target toggle for a period, and
// resets it after a duration.
//
// * It cannot be used until it resets
// * If it is grouped in a set with other switches, it cannot be used while another is active
// * It is assumed to be the only thing togging its target, and does
//   not explicitly reflect its target or have to coordinate to prevent fighting
UCLASS()
class TAT_API ATATTimedSwitch : public ATATInteractableToggle
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   ATATTimedSwitch();

   UFUNCTION(BlueprintPure)
   float GetSecondsUntilReset() const;

   UFUNCTION(BlueprintPure)
   float GetResetProgress() const;

   UFUNCTION(BlueprintCallable)
   void SyncTimelineForReset(UTimelineComponent* timeline) const;

#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif

protected:
   // Called when the game starts or when spawned
   virtual void BeginPlay() override;

   virtual bool _TryPriorityInteractPrompt(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual bool _TryPriorityStartInteract(ACharacter* interactingCharacter, FInteractStartResult& outResult) override;
   
   virtual void ToggleForInteraction(ACharacter* interactingCharacter) override;
   virtual void _OnStateChanged(bool isOn, bool wasRecent) override;

   void _AuthorityOnResetTimerElapsed();

   float _GetResetAge() const;

private:
   // Duration in seconds before the switch resets
   // NOTE: May be dynamic in future if it depends on the target (and thus not here)
   UPROPERTY(EditAnywhere, Category = Switch, meta = (Units = "Seconds", UIMin = 1, ClampMin = 1))
   float _resetDuration = 10.f;

   // What to do to the target when the switch is active
   UPROPERTY(EditAnywhere, Category=Switch)
   ETATTimedSwitchBehavior _targetBehavior = ETATTimedSwitchBehavior::TurnOffTarget;
   
   // [Optional] The toggle that this switch changes
   // TODO: replace with resolver for consistency?
   UPROPERTY(EditInstanceOnly, Category = Switch, meta = (AllowedClasses="/Script/OSEInteraction.OSEToggleInterface"))
   TObjectPtr<AActor> _targetToggle;

   UPROPERTY(EditInstanceOnly, Category = Switch, meta = (ExcludeBaseStruct))
   TInstancedStruct<FTATToggleResolver> _targetResolver;

   UPROPERTY(Transient)
   TScriptInterface<IOSEToggleInterface> _resolvedTarget;

   // Message shown in prompt when the switch is active and still resetting
   // If time is includes, it will use the {Seconds} format parameter
   UPROPERTY(EditDefaultsOnly, Category = Interactable)
   FText _resettingErrorMessage;

   // Whether to include the seconds remaining in the resetting error message
   UPROPERTY(EditDefaultsOnly, Category = Interactable)
   bool _includeTimeInResettingMessage = false;

   TTATParameterizedTextCache<int> _resettingMessageCache;

   FTimerHandle _resetTimerHandle;
};
