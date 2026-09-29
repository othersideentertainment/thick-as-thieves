// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Interactables/InteractableInterface.h"
#include "Interactables/InteractionTargetStrategy.h"
#include "TATInteractionTargeterComponent.generated.h"

class IInteractableInterface;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractionTargetChanged, TScriptInterface<IInteractableInterface>, target);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FInteractionPromptChanged, const FInteractPrompt&, prompt);

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TAT_API UTATInteractionTargeterComponent : public UActorComponent
{
   GENERATED_BODY()

public:   
   // Sets default values for this component's properties
   UTATInteractionTargeterComponent();

   UPROPERTY(EditDefaultsOnly)
   float MaxInteractionRange = 150.0f;

   /// If present, will query the interactable to see if it supports us
   /// If not present, we will assume we can interact with it (previous behavior)
   UPROPERTY(EditDefaultsOnly, meta = (Categories = "Ability.Interact.Interactor"))
   FGameplayTag InteractionFilterTag;

   // Whether to do up to four fallback directional traces on each side of the original interact trace
   UPROPERTY(EditDefaultsOnly, Category = "Extra Aiming")
   bool UseExtraDirectionalTraces = true;

   // Use additional sticky trace if the last interaction found is the close to the current aim
   // (This is important, since the direction traces could miss things, so this keeps it from causing
   // flickering, and may provide some hysteresis depending on the config)
   UPROPERTY(EditDefaultsOnly, Category = "Extra Aiming", meta = (DisplayAfter = DirectionalTraceOffsetDegrees))
   bool UseStickyTarget = true;

   // offset in degrees for extra directional traces
   UPROPERTY(EditDefaultsOnly, Category = "Extra Aiming", meta = (EditCondition = "UseExtraDirectionalTraces"))
   float DirectionalTraceOffsetDegrees = 4.0f;

   // max allowed angle between current direction and the previous sticky target
   UPROPERTY(EditDefaultsOnly, Category = "Extra Aiming", meta = (EditCondition = "UseStickyTarget"))
   float StickyTargetToleranceDegrees = 8.0f;

   // Extra range when using sticky interact target
   UPROPERTY(EditDefaultsOnly, Category = "Extra Aiming", meta = (EditCondition = "UseStickyTarget"))
   float StickyTargetBonusRange = 30.0f;

protected:
   // Called when the game starts
   virtual void BeginPlay() override;

public:   
   // Called every frame
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   UFUNCTION(BlueprintCallable, BlueprintPure)
   TScriptInterface<IInteractableInterface> GetCurrentTarget() const;

   UFUNCTION(BlueprintCallable, BlueprintPure)
   const FInteractPrompt& GetCurrentPrompt() const { return _currentPrompt; }

   UPROPERTY(BlueprintAssignable)
   FInteractionTargetChanged OnTargetChanged;

   UPROPERTY(BlueprintAssignable)
   FInteractionPromptChanged OnPromptChanged;

   UFUNCTION(BlueprintCallable)
   void OverrideStrategy(TScriptInterface<IInteractionTargetStrategy> strategy) { _strategyOverride = strategy.GetObject(); }

   UFUNCTION(BlueprintCallable)
   void RevertStrategy(TScriptInterface<IInteractionTargetStrategy> strategy) { if(_strategyOverride == strategy.GetObject()) _strategyOverride = nullptr; }


   bool IsTargetingSuppressed() const { return _targetingSuppressed; }
   void SetTargetingSuppressed(bool suppressTargeting);


   bool IsTargetingFrozen() const { return _freezeTargetingRequests.Num() > 0; }

   UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm="source"))
   void RequestTargetingFreeze(const FName& source);
   UFUNCTION(BlueprintCallable, meta=(AutoCreateRefTerm="source"))
   void RemoveTargetingFreeze(const FName& source);
   UFUNCTION(BlueprintCallable)
   void ClearTargetingFreeze();

private:
   void SetNewTarget(TScriptInterface<IInteractableInterface> newTarget);
   void RefreshPrompt(ACharacter* ownerCharacter);
   float _GetCurrentInteractRange() const;

   TScriptInterface<IInteractableInterface> _FindNewTarget(ACharacter* ownerCharacter);

   bool _CanInteractWithTarget(ACharacter* ownerCharacter, UObject* target) const;

   const FCollisionResponseParams& _GetTraceResponseParams() const;

   // Don't bother replicating to other clients
   UPROPERTY(Transient)
   TScriptInterface<IInteractableInterface> _targetInteractable;

   UPROPERTY(Transient)
   UObject* _strategyOverride;


   bool _targetingSuppressed = false;

   FInteractPrompt _currentPrompt;

   uint32 _suppressTargetingCounter;
   TArray<FName> _freezeTargetingRequests;

   TOptional<FVector> _lastStickyLocation;
};
