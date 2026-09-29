// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATInteractionGateInterface.h"

// ue
#include "GameFramework/Actor.h"

#include "TATExclusiveToggleGroup.generated.h"

class AOSESyncedToggle;

// Actor that maintains a group of toggles, one of which is active
// Used by UTATExclusiveToggleRequirementComponent
UCLASS()
class TAT_API ATATExclusiveToggleGroup : public AActor
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   ATATExclusiveToggleGroup();

   void RegisterToggle(AOSESyncedToggle* toggle);

   const AOSESyncedToggle* GetActiveToggle() const { return _activeToggle; }
   FSimpleMulticastDelegate OnActiveToggleChanged;

#if WITH_EDITOR
   void AddEditorVisToggle(AActor* actor) { _editorVisToggles.AddUnique(actor); }
   void RemoveEditorVisToggle(AActor* actor) { _editorVisToggles.RemoveSingleSwap(actor); }
   TConstArrayView<TWeakObjectPtr<AActor>> GetEditorVisToggles() const { return _editorVisToggles; }
#endif

private:

   UFUNCTION()
   void _OnToggleStateChanged(bool is_on);
   
   const AOSESyncedToggle* _FindActiveToggle() const;
   void _UpdateActiveToggle();
   
   UPROPERTY(Transient)
   TArray<TObjectPtr<const AOSESyncedToggle>> _toggles;

   UPROPERTY(Transient)
   TObjectPtr<const AOSESyncedToggle> _activeToggle;

#if WITH_EDITORONLY_DATA
   UPROPERTY(Transient)
   TObjectPtr<class UTATExclusiveToggleGroupVisComponent> _visComponent = nullptr;

   TArray<TWeakObjectPtr<AActor>> _editorVisToggles;
#endif
};

// Prevents interaction if another toggle in the same group is active
UCLASS(DisplayName="Exclusive Toggle Requirement", meta = (BlueprintSpawnableComponent))
class TAT_API UTATExclusiveToggleRequirementComponent : public UActorComponent, public ITATInteractionGateInterface
{
   GENERATED_BODY()

public:
   virtual void BeginPlay() override;
   
   virtual bool CanEverBlockInteraction() const override { return true; }
   virtual bool IsInteractionBlocked() const override;
   virtual bool ShowMessageWhenBlocked() const override { return  _showMessageWhenBlocked; }
   virtual void AddToPrompt(FInteractPrompt& prompt) const override;

   UFUNCTION(BlueprintPure)
   bool IsOtherToggleActive() const;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnOtherToggleActiveChanged, bool, isOtherToggleActive);
   UPROPERTY(BlueprintAssignable)
   FOnOtherToggleActiveChanged OnOtherToggleActiveChanged;
   
   const ATATExclusiveToggleGroup* GetToggleGoup() const { return _toggleGroup; }

protected:
   
#if WITH_EDITOR
   virtual void OnRegister() override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void _RefreshEditorVisToggleGroup();
#endif
   
private:
   void _OnActiveToggleChanged();
   
   UPROPERTY(EditInstanceOnly, Category="Toggle Group")
   TObjectPtr<ATATExclusiveToggleGroup> _toggleGroup = nullptr;

#if WITH_EDITORONLY_DATA
   UPROPERTY(Transient)
   TObjectPtr<ATATExclusiveToggleGroup> _editorVisGroup = nullptr;
#endif

   UPROPERTY(EditDefaultsOnly, Category="Toggle Group", meta = (InlineEditConditionToggle))
   bool _showMessageWhenBlocked = false;

   // Message to show when other toggles are in use
   UPROPERTY(EditDefaultsOnly, Category="Toggle Group", meta=(EditCondition = "_showMessageWhenBlocked"))
   FText _blockedMessage;

   bool _isOtherToggleActive = false;
};

// Just for hooking a component visualizer to
UCLASS()
class TAT_API UTATExclusiveToggleGroupVisComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATExclusiveToggleGroupVisComponent();
};
