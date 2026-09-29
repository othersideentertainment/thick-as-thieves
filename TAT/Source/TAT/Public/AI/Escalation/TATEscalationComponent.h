// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once
// tat
#include "TATEscalationState.h"

// ue
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"

#include "TATEscalationComponent.generated.h"

class UTATEscalationDataAsset;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEscalationStateChanged, ETATEscalationState, newEscalationState);
UCLASS()
class TAT_API UTATEscalationComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   virtual void BeginPlay() override;

   UFUNCTION(BlueprintCallable)
   void SetState(ETATEscalationState state);

   UFUNCTION(BlueprintCallable)
   ETATEscalationState GetCurrentState() const { return _currentState; }
   
   UPROPERTY(BlueprintAssignable)
   FOnEscalationStateChanged OnEscalationStateChanged;
protected:
   UPROPERTY(EditDefaultsOnly, Category = "Modifiers")
   TMap<ETATEscalationState, TObjectPtr<UTATEscalationDataAsset>> EscalationModifierDataAssets;

   UTATEscalationDataAsset* GetModifierDataForState(ETATEscalationState state);
   
   void EnterState(const UTATEscalationDataAsset* modifierDataAsset) const;
   void ExitState(const UTATEscalationDataAsset* modifierDataAsset) const;
private:   
   ETATEscalationState _currentState { ETATEscalationState::None };
};
