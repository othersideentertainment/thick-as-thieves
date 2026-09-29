// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Items/ItemActor.h"

// ue4
#include "GameplayTagContainer.h"

#include "TATItemActor.generated.h"

class UTATItemInfo;

UCLASS(Abstract)
class TAT_API ATATItemActor : public AItemActor
{
   GENERATED_BODY()
   
public:

   ATATItemActor();

   // from AActor
   virtual void PostLoad() override;
   virtual void PostActorCreated() override;
   virtual void PostInitializeComponents() override;
   virtual void Tick(float deltaTime) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // from IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt) override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void ShowHighlight_Implementation(bool showHighlight) override;

   UFUNCTION(BlueprintCallable, Category = "Item")
   bool HasRoomForItem(ACharacter* interactingCharacter) const;

   UFUNCTION(BlueprintPure, Category = "Item")
   TSubclassOf<UTATItemInfo> GetItemInfo() const { return Info; }

   UFUNCTION(BlueprintGetter, Category = "Item")
   int32 GetStackCount() const { return _stackCount; }

   void AuthoritySetStackCount(int32 newStackCount);

   float GetDropDistance() const { return DropDistance; }

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

protected:

   void _StartPendingTake();

   UFUNCTION(BlueprintImplementableEvent, Category = "Item")
   void OnUpdateShowing(bool isShowing);

   UFUNCTION(BlueprintImplementableEvent, Category = "Item")
   void OnDisableCollision();

   UFUNCTION(BlueprintImplementableEvent, Category = "Item")
   void OnStackCountChanged();

   UFUNCTION(BlueprintImplementableEvent, Category = "Item")
   void BP_OnFixedDropAuthority(const FVector& dropOrigin, const FVector& intendedDropDestination);

   bool _IsBeingTaken() const { return _takingCharacter != nullptr; }
   FText _GetCachedTakePrompt();
   FText _ComputeTakePrompt() const;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item")
   TSubclassOf<UTATItemInfo> Info;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Item", Meta = (ClampMin = "1", UIMin = "1", ScriptName = "_defaultStackCount"))
   int32 DefaultStackCount = 1;

   UPROPERTY(EditDefaultsOnly, Category = "Item|Pickup")
   float TakeSpeed = 10.0f;

   UPROPERTY(EditDefaultsOnly, Category = "Item|Pickup")
   float CloseDistance = 10.0f;

   UPROPERTY(EditDefaultsOnly, Category = "Item|Pickup")
   float DropDistance = 100.0f;

   UPROPERTY(EditDefaultsOnly, Category = "Item|Pickup")
   float DestroyTime = 5.0f;

   UPROPERTY(EditDefaultsOnly, Category = "Item|Pickup", meta = (Categories = "InteractAnimation.Instant"))
   FGameplayTag TakeInteractAnimationTag;

   UPROPERTY(EditDefaultsOnly, Category = "Item|Pickup")
   float TakeDelay;

   virtual void _OnDroppedAuthority(const FVector& dropOrigin, const FVector& intendedDropDestination, APawn* droppingPawn) override;

private:

   void _StartTakeAnimation();
   bool _IsInteractableInCurrentMission() const;


   UFUNCTION()
   void _OnRep_StackCount();
   UFUNCTION()
   void _OnRep_TakingCharacter(ACharacter* oldTakingCharacter);
   void _DestroySelf();
   void _TickLootTakingAnimation(float deltaTime);

   UPROPERTY(BlueprintGetter = "GetStackCount", Category = "Item", ReplicatedUsing = _OnRep_StackCount)
   int32 _stackCount = 1;

   UPROPERTY(Transient, ReplicatedUsing = _OnRep_TakingCharacter)
   ACharacter* _takingCharacter = nullptr;

   UPROPERTY(Transient)
   ACharacter* _pendingTaker = nullptr;

   FText _takePromptCache;
};
