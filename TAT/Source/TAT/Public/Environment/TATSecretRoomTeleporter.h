// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once


// ose
#include "Interactables/InteractableInterface.h"

// ue
#include "CoreMinimal.h"
#include "Engine/NavigationObjectBase.h"
#include "GameFramework/Actor.h"

#include "TATSecretRoomTeleporter.generated.h"

// An interactable that teleports the player to a target location
//
// For use in secret rooms in contracts
UCLASS()
class TAT_API ATATSecretRoomTeleporter : public AActor, public IInteractableInterface
{
   GENERATED_BODY()

public:
   // Sets default values for this actor's properties
   ATATSecretRoomTeleporter();
   
   virtual void BeginPlay() override;

#if WITH_EDITOR
   virtual void CheckForErrors() override;
#endif

   // Begin IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;
   // End IInteractableInterface
protected:
   // Hook called when visibility changes due to thief-vision/vistara
   // Blueprint is assumed to be off by default until called
   UFUNCTION(BlueprintImplementableEvent)
   void BP_OnVisibilityChanged(bool isVisible);
   
#if WITH_EDITORONLY_DATA
   UPROPERTY(Transient)
   TObjectPtr<class UTATActorDependencyVisComponent> _visComponent = nullptr;
#endif
   
#if WITH_EDITOR
   virtual void PostRegisterAllComponents() override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void _RefreshEditorDependency();
#endif

   UFUNCTION()
   void _OnLocalThiefVisionChanged(APlayerController* controller, bool thiefVisionEnabled);
   
   void _TryExecutePreTeleportCue(ACharacter* interactingCharacter);
   void _TryExecutePostTeleportCue(ACharacter* interactingCharacter);

   UPROPERTY(EditDefaultsOnly, Category = Interactable)
   FText _interactPrompt;

   // Whether the teleporter requires thief vision to see and interact with it
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Teleporter, meta = (BlueprintProtected))
   bool _requireThiefVision = false;

   // Gameplay cue executed at the initial player location when starting the teleport (predicted)
   UPROPERTY(EditDefaultsOnly, Category = Teleporter, meta = (Categories = "GameplayCue"))
   FGameplayTag _preTeleportCue;
   
   // Gameplay cue executed at the final teleport location after the teleport (not predicted)
   UPROPERTY(EditDefaultsOnly, Category = Teleporter, meta = (Categories = "GameplayCue"))
   FGameplayTag _postTeleportCue;

#if WITH_EDITORONLY_DATA
   // The minimum number of targets that a teleporter should have
   // (backed by map-check)
   UPROPERTY(EditDefaultsOnly, Category = Teleporter)
   int _minimumTargetCount = 2;
#endif

   // The targets to teleport to
   // Will currently try them in order
   UPROPERTY(EditInstanceOnly, Category = Teleporter)
   TArray<TObjectPtr<class ATATSecretRoomTeleportTarget>> _targets;
};


// A marker actor where the player could teleport to from a secret room teleporter
//
// Essentially looks like a PlayerStart without being one
UCLASS()
class TAT_API ATATSecretRoomTeleportTarget : public ANavigationObjectBase
{
   GENERATED_BODY()

public:
   ATATSecretRoomTeleportTarget();

private:
#if WITH_EDITORONLY_DATA
   UPROPERTY()
   TObjectPtr<class UArrowComponent> _arrowComponent;
   
   UPROPERTY(Transient)
   TObjectPtr<class UTATActorDependencyVisComponent> _visComponent = nullptr;
#endif
};
