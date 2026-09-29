// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"

// tat
#include "Items/TATItemInventoryComponent.h"

// ose
#include "Interactables/InteractableInterface.h"
#include "Items/ToolVisuals.h"

#include "TATPickpocketableComponent.generated.h"

class AOSECharacterBase;
class UCapsuleComponent;

class UTATItemInfo;
class UTATItemInventoryComponent;

// Similar to FToolVisuals, but:
// 1. Uses static rather than skeletal mesh, and
// 2. Plays nicer with components that are not added to an instance on the fly
USTRUCT(BlueprintType)
struct TAT_API FPickpocketableVisuals
{
   GENERATED_BODY()

public:

   FPickpocketableVisuals();

   // Component events
   UStaticMeshComponent* CreateAndRegisterMeshComponent(AActor* owner, USceneComponent* parent) const;

   /// Location of the component relative to its parent
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Transform)
   FVector RelativeLocation;

   /// Rotation of the component relative to its parent
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Transform)
   FRotator RelativeRotation;

   /// Scale of the component relative to its parent
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Transform)
   float RelativeUniformScale;

   /// Attach point on the parent mesh
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Transform)
   FName AttachPoint;

   /// Perspective to render the mesh
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
   EMeshPerspective Perspective;

   /// The skeletal mesh used by this item
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Mesh)
   TSoftObjectPtr<UStaticMesh> MeshAsset;
};

struct FPickpocketVisualsEntry
{
   TWeakObjectPtr<UTATItemInfo> Item;
   TWeakObjectPtr<UStaticMeshComponent> Mesh;
};

// A component for something that can be pickpocketed
UCLASS(Blueprintable, ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class TAT_API UTATPickpocketableComponent : public USceneComponent, public IInteractableInterface
{
   GENERATED_BODY()

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPickpocketableItemsChanged, const UTATItemInfo*, item);

public:   
   // Sets default values for this component's properties
   UTATPickpocketableComponent();

   // UActorComponent interface
   virtual void OnComponentDestroyed(bool destroyingHierarchy) override;

   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual bool EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;

public:   
   // Called every frame
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   void GetAttachedItems(TArray<UTATItemInfo*>& items) const;

public:
   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket|Interact Capsule")
   float CapsuleHalfHeight;

   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket|Interact Capsule")
   float CapsuleRadius;

   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket|Interact Capsule")
   FVector CapsuleLocation;

   // position of component when actor is lying down
   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket|Interact Capsule")
   FVector CapsuleLyingDownOffset;

   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket|Interact Capsule")
   FName LyingDownAttachPoint;

   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket|FX", meta = (Categories = "GameplayCue"))
   FGameplayTag PickpocketCueTag;

   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket|Taking")
   float TakeSpeed;

   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket|Taking")
   float CloseDistance;

   // whether to allow taking an item if you already have it
   // NOTE: probably shouldn't go here, but this would get reworked once no longer legacy
   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket")
   bool AllowTakingDuplicates = false;

   // Delay in seconds after taking before the items are removed
   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket|Taking")
   float DelayBeforeRemoveItems;

   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket|Taking")
   float DelayBeforeClearTaker;

   UPROPERTY(EditDefaultsOnly, Category = "Pickpocket|Prompt")
   FText InventoryFullPrompt;

   UPROPERTY(BlueprintAssignable, Category = "Pickpocket")
   FOnPickpocketableItemsChanged OnPickpocketableItemAdded;

   UPROPERTY(BlueprintAssignable, Category = "Pickpocket")
   FOnPickpocketableItemsChanged OnPickpocketableItemRemoved;

protected:
   UFUNCTION(BlueprintImplementableEvent)
   void K2_OnTakenLocally(UTATItemInfo* primaryItemInfo);

   UFUNCTION(BlueprintImplementableEvent)
   void K2_OnStartTake(ACharacter* takingCharacter);

private:
   void _StartTake(ACharacter* takingCharacter);
   void _StartTakeAnimation();
   void _PlayTakingCue(ACharacter* takingCharacter);

   bool _HasRoomForItem(ACharacter* interactingCharacter) const;

   bool _DoesOwnerHaveHoldInteraction(ACharacter* interactingCharacter) const;
   AOSECharacterBase* GetCharacterOwner() const;

   UFUNCTION()
   void _OnRep_TakingCharacter(ACharacter* oldTakingCharacter);

   void _InitInventory();
   UFUNCTION()
   void _OnInventoryChanged();
   void _LoadVisualsForItem(TWeakObjectPtr<UTATItemInfo> itemCdo);
   void _ApplyLoadedVisuals(TWeakObjectPtr<UTATItemInfo> itemCdo);
   void _OnRemoveItemsTimer();
   void _OnClearTakerTimer();
   void _OnLyingDownChanged(bool isLyingDown);
   void _DestroyLeftoverVisuals();
   void _OnTakingCharacterCleared();

   void _CreateInteractCapsule();
   void _AttachInteractCapsule();
   void _DestroyInteractCapsule();

private:
   UPROPERTY(Transient, Replicated, ReplicatedUsing = _OnRep_TakingCharacter)
   ACharacter* _takingCharacter;

   TWeakObjectPtr<UTATItemInventoryComponent> _inventory;
   TArray<FPickpocketVisualsEntry> _takeVisuals;

   UPROPERTY(Transient)
   TArray<FTATInventorySlot> _itemsToTake;

   UPROPERTY()
   UCapsuleComponent* _interactCapsule;

   FTimerHandle _removeItemsTimerHandler;
   FTimerHandle _clearTakerTimerHandler;
};
