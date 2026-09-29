// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/TargetActors/OSEAbilityTargetActor_LineTrace.h"

// ue5
#include "CoreMinimal.h"
#include "Engine/CollisionProfile.h"

#include "TATAbilityTargetActor_ThrowVis.generated.h"


UCLASS()
class TAT_API ATATAbilityTargetActor_ThrowVis : public AOSEAbilityTargetActor_LineTrace
{
   GENERATED_BODY()

public:

   ATATAbilityTargetActor_ThrowVis();

   // From AActor
   virtual void BeginPlay() override;
   virtual void Tick(float deltaSeconds) override;

   UFUNCTION(BlueprintImplementableEvent, BlueprintCosmetic)
   void ShowVisualisation();

   UFUNCTION(BlueprintImplementableEvent, BlueprintPure)
   FVector SuggestLaunchVelocity(FVector startPos, FVector endPos) const;

   /// Computes the path of the thrown actor and updates the visuals for it
   virtual void TickVisualization();

   UFUNCTION(BlueprintNativeEvent, BlueprintCosmetic)
   void UpdateVisualsForPredictedThrow(bool throwHasPath, const TArray<FVector>& throwPath, FHitResult traceHit);
   virtual void UpdateVisualsForPredictedThrow_Implementation(bool throwHasPath, const TArray<FVector>& throwPath, FHitResult traceHit) { }

   UFUNCTION(BlueprintCallable)
   FVector GetStartLocation();

   UFUNCTION(BlueprintCallable, Category="Targeting|OSE")
   static FVector GetEyeRelativePosition(AActor* source, const FVector& offset, bool viewRelative = true);

   // In actor space for now, until it is in view-space-ish
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Visualization)
   bool bViewRelative;

   // In actor space for now, until it is in view-space-ish
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Visualization)
   FVector ThrowStartOffset;

   // Radius of the thrown actor to assume for predictions
   UPROPERTY(BlueprintReadOnly, EditAnywhere, meta = (ClampMin = "0.0", ExposeOnSpawn = true), Category = Visualization)
   float PredictedProjectileRadius = 5.0f;

   /// How long after spawn do we start actually visualising the throw
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ClampMin = "0.0", ExposeOnSpawn = true), Category = Visualization)
   float VisualisationDelay = 0.1f;

   /// Optional override of the gravity used to predict thrown actors
   /// If 0, uses world gravity
   UPROPERTY(BlueprintReadWrite, EditAnywhere, meta = (ExposeOnSpawn = true), Category = Visualization)
   float OverrideGravityZ = 0.0f;

   /// How many hertz to simulate the thrown actor at
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Visualization)
   float SimFrequency = 15.0f;

   /// How long the thrown actor is simulated for at most
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = Visualization)
   float MaxSimTime = 2.0f;

private:

   UFUNCTION()
   void _StartShowingVisualisation();
};
