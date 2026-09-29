// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/WorldActors/TATToolWorldActor_Base.h"
#include "Breakables/TATBreakableActorInfoInterface.h"
#include "Breakables/TATBreakableActorFwd.h"

// ue
#include "AbilitySystemInterface.h"
#include "GameplayTagAssetInterface.h"

#include "TATToolWorldActor_Ward.generated.h"

class UTATWardToolComponent;

UCLASS()
class TAT_API ATATToolWorldActor_Ward
   : public ATATToolWorldActor_Base
   , public IAbilitySystemInterface
   , public IGameplayTagAssetInterface //< breakable
   , public ITATBreakableActorInfoInterface
{
   GENERATED_BODY()

public:
   ATATToolWorldActor_Ward();

   BREAKABLE_ACTOR_DECLARATIONS()

   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type reason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   // From IInteractableInterface
   virtual bool IsInteractable_Implementation(ACharacter* interactingCharacter) const override;

   void AuthoritySetupWardBeforeFinishSpawning(AActor* wardedActor, const FVector& wardBoxExtent, APlayerState* owningPlayerState);

   UFUNCTION(BlueprintPure, Category = "Tool World Actor - Ward")
   float GetServerLifeSpanRemaining() const;

protected:
   UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Replicated, Meta = (ExposeOnSpawn), Category = "Tool World Actor - Ward")
   TObjectPtr<AActor> WardedActor;

   UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Replicated, Meta = (ExposeOnSpawn), Category = "Tool World Actor - Ward")
   FVector WardBoxExtent = FVector::ZeroVector;

   UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Replicated, Meta = (ExposeOnSpawn), Category = "Tool World Actor - Ward")
   TObjectPtr<APlayerState> OwningPlayerState;

   UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Transient, Replicated, Meta = (ExposeOnSpawn), Category = "Tool World Actor - Ward")
   double ServerSpawnTimeSeconds = 0.0;

   UTATWardToolComponent* _GetWardTool() const;

protected:
   // Breakable boilerplate
   UPROPERTY(EditDefaultsOnly)
   TObjectPtr<UAbilitySystemComponent> _abilitySystemComponent = nullptr;
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
   TObjectPtr<UTATBreakableComponent> _breakableComponent = nullptr;
};
