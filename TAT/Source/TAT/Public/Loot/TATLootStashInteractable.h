// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Interactables/InteractableInterface.h"

// ue
#include "CoreMinimal.h"

#include "TATLootStashInteractable.generated.h"

class ACharacter;
class UTATMapActorComponent;
class UTATSpawnerComponent;
class UTATLootInventoryComponent;


UCLASS(HideCategories=(LOD, Misc, Physics, Streaming, Actor))
class TAT_API ATATLootStashInteractable : public AActor, public IInteractableInterface
{
   GENERATED_BODY()

public:
   ATATLootStashInteractable();

   // From AActor
   void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
   virtual void PostInitializeComponents() override;

   virtual void TornOff() override;

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

   /// IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;
   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;
   virtual void GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& outPrompt) override;

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityDepositLoot(ACharacter* depositingCharacter);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   bool AuthorityTrySetInteractingCharacter(AActor* interactingCharacter);
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly)
   void AuthorityClearInteractingCharacter(const AActor* interactingCharacter);
   

protected:
   UFUNCTION(BlueprintImplementableEvent, Category = "TAT|Interactable")
   void BP_OnStartDisappearing();

   UFUNCTION(BlueprintImplementableEvent, Category = "TAT|Interactable")
   void BP_OnStashInUseChanged(bool isInUse);
private:

   UFUNCTION()
   void _OnRep_InteractingCharacter();
   void _NotifyStashInUseChanged();
   
   void _AuthoritySetInteractingCharacter(AActor* interactingCharacter);

   void _AuthorityDepositLoot(UTATLootInventoryComponent* lootInventoryComponent, ACharacter* depositingCharacter);
   UFUNCTION()
   void _AuthorityOnNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);

   void _StartDisappearing();

protected:

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map")
   UTATMapActorComponent* _mapActorComponent = nullptr;

   UPROPERTY(EditAnywhere, Category = "Spawn")
   UTATSpawnerComponent* _spawnerComponent = nullptr;

private:

   UPROPERTY(EditDefaultsOnly, Category = "Map")
   FGameplayTag _stashClosedMapTag;
   
   UPROPERTY(EditDefaultsOnly, Category = "Map")
   FGameplayTag _stashPendingMapTag;
   
   UPROPERTY(EditDefaultsOnly, Category = "Map")
   FGameplayTag _stashOpenMapTag;

   UPROPERTY(EditAnywhere, Category = "Map")
   bool _isMapTracked = true;

   UPROPERTY(EditDefaultsOnly, Category = "Interaction", Meta = (Categories = "Ability"))
   FGameplayTag _interactGameplayEvent;
   
   UPROPERTY(EditDefaultsOnly, Category = "Interaction", Meta = (Categories = "InteractAnimation"))
   FGameplayTag _interactInstantAnimation;

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_InteractingCharacter)
   TWeakObjectPtr<AActor> _interactingCharacter;

   bool _isDisappearing = false;
};
