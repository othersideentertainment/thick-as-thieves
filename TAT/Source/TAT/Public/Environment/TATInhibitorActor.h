// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Environment/TATInhibitableInterface.h"

// ue
#include "CoreMinimal.h"

#include "TATInhibitorActor.generated.h"

UCLASS()
class TAT_API ATATInhibitorActor : public AActor
{
   GENERATED_BODY()

public:
   ATATInhibitorActor();

   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type reason) override;
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

   void AuthoritySetupBeforeFinishSpawning(AActor* inhibitedActor, const FTATInhibitorPlacementInfo& placementInfo, const FLinearColor& color, APlayerState* owningPlayerState);

   /// Returns the number of seconds remaining before this inhibitor is automatically despawned.
   /// Requires that this actor has InitialLifeSpan set to a value greater than zero.
   UFUNCTION(BlueprintPure, Category = "Inhibitor Actor")
   float GetInhibitorLifeSpanRemaining() const;

   /// This inhibitor's type.
   /// Allows inhibitable actors to decide if this inhibitor affects them or not.
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Inhibitor Actor", Meta = (Categories = "Inhibitor"))
   FGameplayTag InhibitorType;

protected:
   /// The actor this inhibitor is affecting
   UPROPERTY(BlueprintReadOnly, Transient, Replicated, Meta = (ExposeOnSpawn), Category = "Inhibitor Actor")
   TObjectPtr<AActor> InhibitedActor;

   /// The physical parameters of the area this inhibitor is affecting.
   /// For example, a door might use a box shape, or a light source might use a point or sphere shape.
   UPROPERTY(BlueprintReadOnly, Transient, Replicated, Meta = (ExposeOnSpawn), Category = "Inhibitor Actor")
   FTATInhibitorPlacementInfo PlacementInfo;

   /// The desired color of the inhibitor actor
   UPROPERTY(BlueprintReadOnly, Transient, Replicated, Meta = (ExposeOnSpawn), Category = "Inhibitor Actor")
   FLinearColor InhibitorColor = FLinearColor::White;

   /// The player state that was responsible for this inhibitor being spawned (if any)
   UPROPERTY(BlueprintReadOnly, Transient, Replicated, Meta = (ExposeOnSpawn), Category = "Inhibitor Actor")
   TObjectPtr<APlayerState> OwningPlayerState;

   /// The server world time this inhibitor was spawned
   UPROPERTY(BlueprintReadOnly, Transient, Replicated, Meta = (ExposeOnSpawn), Category = "Inhibitor Actor")
   double ServerSpawnTimeSeconds = 0.0;
};
