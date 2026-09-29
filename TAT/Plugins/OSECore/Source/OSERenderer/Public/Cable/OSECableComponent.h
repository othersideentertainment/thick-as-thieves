// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "CableComponent.h"
#include "OSECableComponent.generated.h"

/**
 * 
 */
UCLASS(ClassGroup = ("Rendering|OSE"), Abstract, Blueprintable, BlueprintType
   , meta = (BlueprintSpawnableComponent, IsBlueprintBase = "true")
   , hidecategories = (Object, Physics, Activation, "Components|Activation"))
class OSERENDERER_API UOSECableComponent : public UCableComponent
{
   GENERATED_BODY()

public:

   // Begins play for this component. Occurs at level startup or actor spawn
   // This is before BeginPlay (Actor or Component).
   // All Components(that want initialization) in the level will be Initialized on load before any Actor / Component gets BeginPlay.
   virtual void BeginPlay() override;

   // Ends play for this component. Called from AActor::EndPlay only if bHasBegunPlay is true
   virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

   // Called every frame
   virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

public:

   /// Returns true if the cable has been ticked
   UFUNCTION(BlueprintCallable, Category = "Cable|OSE")
   virtual bool HasCableBeenTicked() const { return HasBeenTicked; }

   /// Returns the total length of the cable in world space. This is the actual distance between each particle computed each tick
   UFUNCTION(BlueprintCallable, Category = "Cable|OSE")
   virtual float GetCableTotalLength() const { return TotalLength; }

   /// Returns the world-space particle transform at the specified length in world space. This is based on the total cable length.
   /// If the length is less than zero or greater than the total length, this function will return false.
   /// The transform will always be set to a valid value, clamped to the length.
   UFUNCTION(BlueprintCallable, Category = "Cable|OSE")
   virtual bool GetCableTransformAtLength(FTransform& Transform, float Length) const;

   /// Same as GetCableTransformAtLength, except the length is specified from the end point.
   UFUNCTION(BlueprintCallable, Category = "Cable|OSE")
   virtual bool GetCableTransformAtReverseLength(FTransform& Transform, float ReverseLength) const;

   /// Returns the world-space particle transform at the specified index. If the index is out of range, it will clamp it.
   UFUNCTION(BlueprintCallable, Category = "Cable|OSE")
   virtual FTransform GetCableTransformAtIndex(int32 ParticleIndex) const;

   UFUNCTION(BlueprintCallable, Category = "Cable|OSE")
   virtual void SampleCableTransformsUniformly(float offset, float length, int samples, TArray<FTransform>& transforms);
   UFUNCTION(BlueprintCallable, Category = "Cable|OSE")
   virtual void SampleCableTransformsUniformlyReversed(float offset, float length, int samples, TArray<FTransform>& transforms);

private:

   // True if the cable has been ticked since begin play
   bool HasBeenTicked = false;

   // The total length of the cable in world space. Recomputed each tick.
   float TotalLength = 0.0f;

   // The base class particle array is private, so we have to copy it each tick.
   // This is actually fine, as we also compute distance, and cache them in a
   // separate array as well.
   TArray<FVector> CachedParticles;
   TArray<float> CachedDistances;
};
