// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

#include "CoreMinimal.h"

// tat
#include "AI/SmartObjects/TATActionNodeComponent_IncorrectObjectState.h"
#include "AI/SmartObjects/TATAIIncorrectObjectStateInterface.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"
#include "AI/SmartObjects/TATSmartObjectTagInterface.h"

#include "Interactables/TATVisionPerceptionDevice.h"

#include "TATSecurityTurret.generated.h"

class ATATProjectile;

DECLARE_LOG_CATEGORY_EXTERN(LogTATSecurityTurret, Warning, All);

USTRUCT()
struct FTATSecurityTurretDifficultySettings
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere, Category = "Swivel")
   bool UseMultiplierInsteadOfSeconds { false };
   
   UPROPERTY(EditAnywhere, Category = "TAT|Setup", meta=(EditCondition="UseMultiplierInsteadOfSeconds", EditConditionHides, ClampMin=0.f, ClampMax=1.f))
   float FireRateMultiplier { 1.f };
   
   UPROPERTY(EditAnywhere, Category = "TAT|Setup", meta=(EditCondition="UseMultiplierInsteadOfSeconds == false", EditConditionHides))
   float FireRate { 0.5 };
};

UCLASS(Abstract)
class TAT_API ATATSecurityTurret : public ATATVisionPerceptionDevice
                                 , public IInteractableInterface
                                 , public ITATAIIncorrectObjectStateInterface
                                 , public ITATSmartObjectTagInterface
                                 , public ITATSmartObjectOwnerInterface
{
   GENERATED_BODY()

public:
   ATATSecurityTurret();

protected:
   virtual void BeginPlay() override;
   virtual void PostInitializeComponents() override;
public:
   virtual void Tick(float deltaTime) override;

   UFUNCTION(BlueprintCallable, BlueprintPure, BlueprintImplementableEvent)
   USceneComponent* GetFiringPosition() const;

   UFUNCTION(BlueprintCallable, BlueprintPure)
   AActor* GetTargetForFiring() const;

   virtual bool IsAllowedToSeeActor(const AActor* actor) const override;

   UFUNCTION(BlueprintCallable)
   bool GetProjectileVelocityForTarget(const AActor* target, FVector& outVelocity) const;

   UFUNCTION(BlueprintCallable)
   TSubclassOf<ATATProjectile> GetProjectileType() const { return _projectileType; }

   /// Initial abilities
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="TAT|Setup")
   TArray< class UOSEGameplayAbilitySet* > InitialAbilitySets;

   // begin ITATAIIncorrectObjectStateInterface
   virtual bool AuthorityIsObjectInCorrectState_Implementation(bool allowIgnoringOfState) const override;
   // end ITATAIIncorrectObjectStateInterface
   
   // begin ITATSmartObjectTagInterface
   virtual FGameplayTagCountContainer& GetGameplayTagCountContainer() override;
   // end ITATSmartObjectTagInterface
   
   // begin ITATSmartObjectOwnerInterface
   UFUNCTION(BlueprintCallable)
   virtual UTATSmartObjectComponent* GetSmartObjectComponent() const override { return _incorrectStateActionNodeComponent; }
   // end ITATSmartObjectOwnerInterface
   
   
   // Begin IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* InteractingCharacter) override;
   virtual void ShowHighlight_Implementation(bool bShowHighlight) override;
   // End IInteractableInterface

   virtual void _OnStateChanged(bool bIsOn, bool bWasRecent) override;
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Interaction|Animation")
   FGameplayTag TurnOnAnimationTag;
   UPROPERTY(EditDefaultsOnly, Category="TAT|Interaction|Animation")
   FGameplayTag TurnOffAnimationTag;

   UPROPERTY(EditAnywhere, Category = "TAT|Interaction")
   FText TurnOnPrompt;

   UPROPERTY(EditAnywhere, Category = "TAT|Interaction")
   FText TurnOffPrompt;

protected:
   virtual void SetSettingsForDifficulty(const ETATDifficulty difficulty) override;
   UPROPERTY(EditAnywhere, Category="TAT|Setup")
   bool _allowManuallyChangingPowerState { true };
   
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|AI")
   class UTATActionNodeComponent_IncorrectObjectState* _incorrectStateActionNodeComponent = nullptr;
   
   UPROPERTY(EditDefaultsOnly)
   class UAIPerceptionStimuliSourceComponent* _perceptionStimuliSource = nullptr;
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Setup")
   FGameplayTag _turretFireGameplayTag;

   UPROPERTY(EditDefaultsOnly, Category="TAT|Projectile")
   TSubclassOf<ATATProjectile> _projectileType;
   
   UPROPERTY(EditAnywhere, Category = "TAT|Tuning")
   TMap<ETATDifficulty, FTATSecurityTurretDifficultySettings> _difficultyToTuningMap;
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Tuning")
   float _fireRate {0.5f};

   UPROPERTY(EditDefaultsOnly, Category="TAT|Tuning", meta=(ClampMin=0.f))
   float _targetDistanceUncertainty { 0.f };
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|Tuning", meta=(ClampMin=0.f, ClampMax=1.f))
   float _targetLeadingAmount { 0.5f };

   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<USceneComponent> _rootComponent { nullptr };

   bool _HasActiveAlarm() const;
   virtual bool _ShouldTurnOnWhenRepaired() const override;

   virtual void _AuthorityOnAlarmSystemStateChanged(EAlarmState newState, EAlarmState previousState, bool isRecent) override;
private:
   float _nextTimeAvailableToFire { -1.f };
   float _projectileMaxSpeed { -1.f};
};
