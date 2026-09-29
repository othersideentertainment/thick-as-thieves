// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Kismet/BlueprintAsyncActionBase.h"

// tat
#include "AI/Utility/TATAIStateCombination.h"

#include "TATAsyncTaskAIStatesChanged.generated.h"

class UOSEAlertnessComponent;
class ATATAIController;
class UTATEscalationComponent;

UCLASS(BlueprintType, meta = (ExposedAsyncProxy = AsyncTask))
class TAT_API UTATAsyncTaskAIStatesChanged : public UBlueprintAsyncActionBase
{
   GENERATED_BODY()

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnAIStatesChanged, const FTATAIStateCombination&, previousStates, const FTATAIStateCombination&, currentStates);

   // Listens for an AI state change.
   UFUNCTION(BlueprintCallable, meta = (BlueprintInternalUseOnly = "true", WorldContext = "worldContextObject"), DisplayName = "SpawnAIStateChangeListener")
   static UTATAsyncTaskAIStatesChanged* ListenForStateChanges(UObject* worldContextObject, ATATAIController* aiController);

   // from UBlueprintAsyncActionBase
   virtual void Activate() override;

public:
   // If true, will trigger OnAIAlertnessChanged and OnAIEscalationChanged on Activate(),
   // passing the initial values found for Alertness and Escalation
   UPROPERTY(EditAnywhere, BlueprintReadWrite)
   bool TriggerEventsOnActivation = true;

   // Triggered when an AI's Alertness level changes.
   UPROPERTY(BlueprintAssignable)
   FOnAIStatesChanged OnAIAlertnessChanged;

   // Triggered when an AI's Escalation state changes.
   UPROPERTY(BlueprintAssignable)
   FOnAIStatesChanged OnAIEscalationChanged;

   // You must call this function manually when you want the AsyncTask to end.
   // For UMG Widgets, you would call it in the Widget's Destruct event.
   UFUNCTION(BlueprintCallable)
   void EndTask();

private:
   UPROPERTY()
   FTATAIStateCombination _currentStateCombo;

   UPROPERTY()
   UOSEAlertnessComponent* _alertnessComponent = nullptr;

   UPROPERTY()
   UTATEscalationComponent* _escalationComponent = nullptr;

   UFUNCTION()
   void _OnAlertnessLevelChanged(EAlertnessLevel oldAlertnessLevel, EAlertnessLevel newAlertnessLevel);

   UFUNCTION()
   void _OnEscalationStateChanged(const ETATEscalationState newEscalationState);
};

DECLARE_LOG_CATEGORY_EXTERN(LogAsyncTaskAIStatesChanged, Log, All);
