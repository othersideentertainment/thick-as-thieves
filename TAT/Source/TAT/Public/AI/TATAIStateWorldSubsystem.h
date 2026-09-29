// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ue4
#include "CoreMinimal.h"

// ose
#include "AI/Alertness/AlertnessEnums.h"
#include "AI/Alertness/DetectionEnums.h"

#include "TATAIStateWorldSubsystem.generated.h"

class ATATCharacterAIBase;

UCLASS()
class TAT_API UTATAIStateWorldSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnMaxAlertnessLevelIncreased, EAlertnessLevel, newLevel, const ATATCharacterAIBase*, increasedAI);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxAlertnessLevelDecreased, EAlertnessLevel, newLevel);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxDetectionStateChanged, EActorDetectionState, maxDetectionState);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnMaxDetectionValueChanged, float, maxDetectionValue);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalPlayerMonocularUsageChanged, bool, newUsage);
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalPlayerIsStandingStillChanged, bool, isStandingStill);

public:
   // Begin USubsystem
   virtual void Initialize(FSubsystemCollectionBase& Collection) override;
   virtual void Deinitialize() override;
   // End USubsystem

   void RegisterAICharacter(const ATATCharacterAIBase* character);
   void OnAIAlertnessLevelChanged(const ATATCharacterAIBase* character);

   void SetLocalPlayerIsUsingMonocular(bool isUsingMonocular);

   bool GetLocalPlayerIsUsingMonocular() const { return _isLocalPlayerCurrentlyUsingMonocular; }

   void SetLocalPlayerIsStandingStill(const bool isStandingStill);
   UFUNCTION(BlueprintPure)
   bool GetLocalPlayerIsStandingStill() const { return _isLocalPlayerStandingStill; }

   UPROPERTY(BlueprintAssignable, Category = "TAT|AIStateWorldSubsystem")
   FOnMaxAlertnessLevelIncreased OnMaxAlertnessIncreased;

   UPROPERTY(BlueprintAssignable, Category = "TAT|AIStateWorldSubsystem")
   FOnMaxAlertnessLevelDecreased OnMaxAlertnessDecreased;

   UPROPERTY(BlueprintAssignable, Category = "TAT|AIStateWorldSubsystem")
   FOnMaxDetectionStateChanged OnMaxDetectionStateIncreased;

   UPROPERTY(BlueprintAssignable, Category = "TAT|AIStateWorldSubsystem")
   FOnMaxDetectionStateChanged OnMaxDetectionStateDecreased;

   UPROPERTY(BlueprintAssignable, Category = "TAT|AIStateWorldSubsystem")
   FOnMaxDetectionValueChanged OnMaxDetectionValueIncreased;

   UPROPERTY(BlueprintAssignable, Category = "TAT|AIStateWorldSubsystem")
   FOnMaxDetectionValueChanged OnMaxDetectionValueDecreased;
   
   FOnLocalPlayerMonocularUsageChanged OnLocalPlayerMonocularUsageChanged;

   UPROPERTY(BlueprintAssignable)
   FOnLocalPlayerIsStandingStillChanged OnLocalPlayerIsStandingStillChanged;

private:
   void _UpdateAIStateMaxSeen();
   //FTATAIStateContainer& _FindOrCreateAIStateContainer(const ATATCharacterAIBase* character);

   struct FRegisteredAIState
   {
   public:
      FRegisteredAIState(TWeakObjectPtr<const ATATCharacterAIBase> character)
         : Character(character)
      {}

      void ResetFlags()
      {
         HadAlertness = false;
         HadDetection = false;
      }

      TWeakObjectPtr<const ATATCharacterAIBase> Character;
      bool HadAlertness = false;
      bool HadDetection = false;
   };

   TArray<FRegisteredAIState> _registeredAI;

   FTimerHandle _updateAIStateMaxSeenTimerHandle;
   EAlertnessLevel _maxAlertnessSeen = EAlertnessLevel::Neutral;
   EActorDetectionState _maxDetectionStateSeen = EActorDetectionState::Observing;
   float _maxDetectionValueSeen = 0.0f;

   bool _isLocalPlayerCurrentlyUsingMonocular = false;
   bool _isLocalPlayerStandingStill = false;
};
