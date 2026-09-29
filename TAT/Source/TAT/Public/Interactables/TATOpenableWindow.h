// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATSecurityLockdownComponent.h"
#include "AI/SmartObjects/TATAIIncorrectObjectStateInterface.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"
#include "AI/SmartObjects/TATSmartObjectTagInterface.h"
#include "Interactables/TATLockableToggle.h"
#include "Interactables/TATSupportInteractionByInterface.h"
#include "Perception/AISightTargetInterface.h"

// ue
#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

#include "TATOpenableWindow.generated.h"

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_Object_Broken_Window)

class UTATActionNodeComponent_IncorrectObjectState;
class UAIPerceptionStimuliSourceComponent;
struct FTATAuthorityBreakContext;

UCLASS()
class TAT_API ATATOpenableWindow
   : public ATATLockableToggle
   , public ITATAIIncorrectObjectStateInterface
   , public IAISightTargetInterface
   , public ITATSupportInteractionByInterface
   , public ITATSmartObjectTagInterface
   , public ITATSmartObjectOwnerInterface
{
public:
   // ITATUtilityAITargetingGroupInterface start
   virtual FGameplayTag GetUtilityAITargetingGroup() const override;
   // ITATUtilityAITargetingGroupInterface end

   // ITATSmartObjectOwnerInterface start
   UFUNCTION(BlueprintCallable)
   virtual UTATSmartObjectComponent* GetSmartObjectComponent() const override;
   // ITATSmartObjectOwnerInterface end

private:
   GENERATED_BODY()

   ATATOpenableWindow();

   // from AActor
   virtual void PostInitializeComponents() override;
   virtual void BeginPlay() override;

   // from ITATAIIncorrectObjectStateInterface
   virtual bool AuthorityIsObjectInCorrectState_Implementation(bool allowIgnoringOfState) const override;

   // from IAISightTargetInterface
   virtual bool CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor = nullptr,
      const bool* wasVisible = nullptr, int32* userData = nullptr) const override;

   // from ITATSupportInteractionByInterface
   virtual bool DoesSupportInteractionBy_Implementation(ACharacter* interactor, const FGameplayTag& interactorIdentity) const override;
   
#if WITH_EDITOR
   // from AActor
   virtual void CheckForErrors() override;
#endif

   // ITATSmartObjectTagInterface start
   virtual FGameplayTagCountContainer& GetGameplayTagCountContainer() override;
   // ITATSmartObjectTagInterface end

protected:   
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TAT|AI")
   UTATActionNodeComponent_IncorrectObjectState* _incorrectStateActionNodeComponent = nullptr;
   
   // Managed by '_incorrectStateActionNodeComponent' to make the actor only visible
   // when in an incorrect state.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|AI")
   UAIPerceptionStimuliSourceComponent* _perceptionStimuliSource = nullptr;

   UPROPERTY(EditAnywhere, Category="TAT|Security", meta=(DisplayPriority=2))
   UTATSecurityLockdownComponent* _securityLockdownComponent = nullptr;

   UPROPERTY(EditDefaultsOnly, Category = "Interactable", meta = (Categories = "Ability.Interact.Interactor"))
   FGameplayTagContainer _allowedInteractors;

   virtual FTATLockableToggleAllowedDirections _GetAllowedDirections() const override;

   virtual void _OnStateChanged(bool bIsOn, bool bWasRecent) override;

   UFUNCTION()
   void _OnSecurityLockdownStateChanged(bool bIsInLockdown);
   
   UFUNCTION(BlueprintImplementableEvent, Category=Door, meta = (BlueprintProtected=true))
   void UpdateLockdownVisual(bool bIsInLockdown);

private:
   // Settings used when locked-down (i.e. after linked alarms have been triggered)
   UPROPERTY(EditAnywhere, Category = "Lock", meta=(DisplayName = "Lockdown Allowed Directions", EditCondition="_overrideLockdownAllowedDirections", DisplayAfter="_normalAllowedDirections"))
   FTATLockableToggleAllowedDirections _lockdownAllowedDirections;

   UPROPERTY(EditAnywhere, Category = "Lock", meta=(InlineEditConditionToggle))
   bool _overrideLockdownAllowedDirections = false;

   // This sticks indefinitely
   UPROPERTY(Transient, Replicated)
   bool _lockedDown = false;
   
};
