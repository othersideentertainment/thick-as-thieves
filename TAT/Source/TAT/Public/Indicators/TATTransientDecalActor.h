// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "GameFramework/Actor.h"

// tat
#include "Indicators/TATClientProxyActorInterface.h"

#include "TATTransientDecalActor.generated.h"

class UDecalComponent;

UENUM()
enum class ETATTransientDecalDirectionHintMode
{
   Custom = 0,         // Don't actually move any components, just fire the direction hint changed event for VFX purposes.
   MoveActor,          // Move the whole actor.
   MoveDecalComponent, // Only move the decal component (and anything attached to it). Useful for keeping another component stationary.
};

/// Actor class used for spawning decal-based indicators (like footsteps) at runtime.
/// This manages visibility transitions and supports a "direction hint" mode where it interpolates the position forward to show directionality.
UCLASS(Blueprintable)
class TAT_API ATATTransientDecalActor : public AActor, public ITATClientProxyActorInterface
{
   GENERATED_BODY()

   ATATTransientDecalActor();

public:
   // From AActor
   virtual void BeginPlay() override;
   virtual void Tick(float deltaSeconds) override;

   // From ITATClientProxyActorInterface
   virtual void OnSpawnedAsClientProxy_Implementation(float remainingLifeSpan, float indicatorLifeSpan, const FTATClientProxySpawnParams& spawnParams) override;
   virtual void OnDestroyClientProxy_Implementation() override;
   virtual void OnClientProxyActorSetVisible_Implementation(bool newVisible) override;
   virtual void OnClientProxyActorLifeSpanRefreshed_Implementation(float newRemainingLifeSpan) override;

   /// Checks if this actor was spawned as a client proxy (eg. thief vision indicator) or not.
   UFUNCTION(BlueprintPure, Category = "Transient Decal Actor")
   FORCEINLINE bool IsClientProxy() const { return _isClientProxy; }

   /// Sets a new target opacity for the decal.
   UFUNCTION(BlueprintCallable, Category = "Transient Decal Actor")
   void SetTargetOpacity(float newTargetOpacity);

   UFUNCTION(BlueprintPure, Category = "Transient Decal Actor")
   FORCEINLINE float GetMaximumOpacity() const { return _maximumOpacity; }

   /// Sets the maximum opacity that the decal can be set to.
   /// Note that if _autoUpdateMaximumOpacity is enabled, this will be overwritten by the auto-computed value.
   UFUNCTION(BlueprintCallable, Category = "Transient Decal Actor")
   void SetMaximumOpacity(float newMaximumOpacity);

   /// Gets the decal's dynamic material instance, creating it if it does not exist yet.
   UFUNCTION(BlueprintCallable, Category = "Transient Decal Actor")
   UMaterialInstanceDynamic* GetOrCreateDecalMaterialInstanceDynamic();

   /// If this actor was spawned as a client proxy with a finite lifespan, returns the time remaining before destruction and the amount of time it will be alive.
   /// Note that client proxies can be refreshed, which resets their lifespan.
   /// Returns false if the actor isn't a client proxy, or the client proxy has an infinite lifespan.
   UFUNCTION(BlueprintPure, Category = "Transient Decal Actor")
   bool GetRemainingAndTotalLifeSpan(float& outRemainingLifeSpan, float& outTotalLifeSpan) const;

   UFUNCTION(BlueprintPure, Category = "Transient Decal Actor")
   FORCEINLINE bool IsAutoUpdatingMaximumOpacity() const { return _autoUpdateMaximumOpacityTimer.IsValid(); }

   /// Enables or disables auto-updating the maximum opacity over the lifespan of the actor.
   UFUNCTION(BlueprintCallable, Category = "Transient Decal Actor")
   bool SetAutoUpdateMaximumOpacity(bool enabled);

   FORCEINLINE UDecalComponent* GetDecalComponent() const { return _decalComponent; }

protected:
   /// Enables the direction hint (where we slowly move this actor in the specified direction)
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Direction Hint")
   bool _enableDirectionHint = false;

   /// When the direction hint is enabled, the distance it will travel
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Direction Hint", meta = (ClampMin = "0.0", UIMin = "0.0"))
   float _directionHintDistance = 25.0f;

   /// When using a direction hint, how long should it take to move the indicator from the start position to the end?
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Direction Hint", meta = (ClampMin = "0.0", UIMin = "0.0", ForceUnits = "s"))
   float _directionHintMovementDurationSeconds = 5.0f;

   /// What should be moved when using the direction hint mode.
   /// Moving just the decal component can be useful for having a footstep decal have some movement while keeping other VFX components stationary.
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Direction Hint")
   ETATTransientDecalDirectionHintMode _directionHintMovementMode = ETATTransientDecalDirectionHintMode::MoveActor;

   /// When the direction hint reaches the end, should it auto-reset back to the beginning and continue?
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Direction Hint")
   bool _directionHintAutoReset = true;

   /// Should we automatically update the maximum opacity over the lifetime of the actor?
   /// Note that enabling this simply turns it on at spawn. You can call SetAutoUpdateMaximumOpacity to dynamically turn it on or off when needed.
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Auto-Update Maximum Opacity")
   bool _autoUpdateMaximumOpacity = false;

   /// If auto-update maximum opacity is enabled, the normalized lifespan range we should adjust the opacity over.
   /// (If the decal's lifespan is 60 seconds, a value of 0.75 is equal to 45 seconds, and 0.5 is equal to 30 seconds)
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Auto-Update Maximum Opacity")
   FVector2D _autoUpdateMaximumOpacityNormalizedLifespanRange = FVector2D(1.0f, 0.0f);

   /// If auto-update maximum opacity is enabled, the maximum opacity range we'll apply over the specified lifespan.
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Auto-Update Maximum Opacity")
   FVector2D _autoUpdateMaximumOpacityOpacityRange = FVector2D(1.0f, 0.2f);

   /// If auto-update maximum opacity is enabled, how frequently (in seconds) to update the maximum opacity.
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Auto-Update Maximum Opacity", meta = (ClampMin = "0.1", UIMin = "0.1", ForceUnits = "s"))
   float _autoUpdateMaximumOpacityUpdateRate = 1.0f;

   /// When changing the opacity, how long should it take to fully interpolate from 0 to 1
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Transient Decal Actor", meta = (ClampMin = "0.0", UIMin = "0.0", ForceUnits = "s"))
   float _opacityFadeInTimeSeconds = 0.5f;

   /// When changing the opacity, how long should it take to fully interpolate from 1 to 0
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Transient Decal Actor", meta = (ClampMin = "0.0", UIMin = "0.0", ForceUnits = "s"))
   float _opacityFadeOutTimeSeconds = 0.5f;

   /// Parameter name in the decal material to use to change the decal's opacity
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Transient Decal Actor")
   FName _materialOpacityParameterName = "Opacity";

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Transient Decal Actor")
   USceneComponent* _rootComponent = nullptr;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Transient Decal Actor")
   UDecalComponent* _decalComponent = nullptr;

   /// Event fired when the decal's opacity changes.
   /// This can be used to link the opacity of other VFX (sprites, particle systems, etc.) to the decal's opacity.
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOpacityChangeEvent, float, newValue);
   UPROPERTY(BlueprintAssignable)
   FOpacityChangeEvent OnOpacityChanged;

   /// Event fired when the decal's direction hint position changes.
   /// This can be used to link other VFX (sprites, particle systems, etc.) to the decal's direction hint state.
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FDirectionHintChangeEvent, float, newValue, const FVector&, newWorldLocation);
   UPROPERTY(BlueprintAssignable)
   FDirectionHintChangeEvent OnDirectionHintChanged;

private:
   UFUNCTION()
   void _OnAutoMaxOpacityUpdate();

   void _SetOpacityDirect(float newOpacity);
   void _SetDirectionHintDirect(float newNormalizedPos);

   void _OnOpacityChanged(float newOpacity);
   void _OnDirectionHintChanged(float newNormalizedPos, const FVector& newWorldLocation);

   UMaterialInstanceDynamic* _ConstructDynamicMaterialInstance();

   UPROPERTY(Transient)
   UMaterialInstanceDynamic* _decalMaterial = nullptr;

   /// Was this actor spawned as a client proxy?
   bool _isClientProxy = false;

   /// World time at spawn (or when the lifespan was last reset)
   float _worldTimeAtSpawnOrRefresh = 0.0f;

   /// If this is a client proxy actor, this is the client proxy equivalent of an actors InitialLifeSpan
   float _clientProxyLifespan = 0.0f;

   /// What was the client proxy's remaining lifespan at the last spawn or refresh event?
   float _clientProxyLifespanRemainingAtSpawnOrRefresh = 0.0f;

   /// Target and current opacity - any time these values don't match, we interpolate from current to target.
   float _targetOpacity = 0.0f;
   float _currentOpacity = 0.0f;

   /// Maximum opacity value
   float _maximumOpacity = 1.0f;

   /// Timer to tick when auto-updating maximum opacity
   FTimerHandle _autoUpdateMaximumOpacityTimer;

   /// When using the direction hint, the start and end positions
   FVector _directionHintStart = FVector::ZeroVector;
   FVector _directionHintEnd = FVector::ZeroVector;

   /// Value between 0 and 1 that represents how far along the path the direction hint is (target and current)
   float _targetDirectionHintNormalizedPos = 1.0f;
   float _currentDirectionHintNormalizedPos = 0.0f;

   /// If we're currently resetting the direction hint back to the beginning
   bool _directionHintIsResetting = false;

};
