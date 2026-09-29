// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Interactables/Electrical/TATPowerNetworkInterface.h"
#include "Character/TATCharacterBaseChangeNotifyInterface.h"
#include "Environment/TATSplineUtilities.h"

// ue
#include "ActiveGameplayEffectHandle.h"
#include "GameplayTagContainer.h"

#include "TATPowerLine.generated.h"

class USplineComponent;
class UTATPowerNetworkComponent;
class UGameplayEffect;

/// Represents a connection between two nodes in a power network made up of actors with power network components.
UCLASS(Blueprintable)
class TAT_API ATATPowerLine
   : public AActor
   , public ITATPowerNetworkInterface
   , public ITATCharacterBaseChangeNotifyInterface
{
   GENERATED_BODY()

public:
   ATATPowerLine();

protected:
   virtual void OnConstruction(const FTransform& transform) override;
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type reason) override;
   virtual void Tick(float deltaSeconds) override;
   virtual void NotifyHit(UPrimitiveComponent* comp, AActor* otherActor, UPrimitiveComponent* otherComp, bool selfMoved, FVector hitLocation, FVector hitNormal, FVector normalImpulse, const FHitResult& hitResult) override;

public:
   // ITATPowerNetworkInterface
   virtual UTATPowerNetworkComponent* GetPowerNetworkComponent() const override { return PowerNetworkComponent; }
   virtual USplineComponent* GetPowerNetworkConnectorSplineComponent() const override { return SplineComponent; }
   virtual TOptional<FVector> GetPowerNetworkWorldLocationForConnectionIndex(int32 index, bool forDebugVis) const override;

   // ITATCharacterBaseChangeNotifyInterface
   virtual void OnCharacterBeginBasing_Implementation(ACharacter* character) override;

   UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Power Line")
   TObjectPtr<UTATPowerNetworkComponent> PowerNetworkComponent;

   UPROPERTY(BlueprintReadOnly, VisibleAnywhere, Category = "Power Line")
   TObjectPtr<USplineComponent> SplineComponent;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Line", Meta = (InlineEditConditionToggle))
   bool UseSplineMesh = true;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Line", Meta = (EditCondition = "UseSplineMesh"))
   FTATSplineMeshConfig SplineMesh;

   /// Gameplay tag to apply to any character while they are standing on a power line
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Line")
   FGameplayTag StandingOnPowerLineGameplayTag;

   /// Apply this gameplay effect to any character that touches the power line during a power surge
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Line|Power Surge")
   TSubclassOf<UGameplayEffect> PowerSurgeGameplayEffect;

   /// Number of stacks to remove when removing power surge gameplay effects.
   /// Set to 0 to disable auto-removing the gameplay effect.
   /// Set to -1 to remove all stacks.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Power Line|Power Surge", Meta = (UIMin = "-1", ClampMin = "-1"))
   int32 PowerSurgeGameplayEffectStacksToRemove = 1;

   /// Manually run the cable simulation one time and apply the results to the spline component.
   ///
   /// Note that this is only useful if AutoApplySplineCableSimulation is disabled.
   UFUNCTION(CallInEditor, Category = "Spline Cable Simulation")
   void RunCableSimulationAndApplyToSpline();

   /// If enabled, the cable simulation will always be run automatically before the blueprint's construction script.
   /// This keeps the spline points in the correct position when moving connected actors around, but prevents manual spline point adjustment.
   ///
   /// If disabled, you can still run the cable simulation as a one-off by clicking "Run Cable Simulation And Apply To Spline".
   /// This is very useful if you want to set up the spline points in a natural place before adjusting them manually.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Cable Simulation")
   bool AutoApplySplineCableSimulation = false;

   /// A multiplier for the "rest length" of each spline segment
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Cable Simulation", Meta = (UIMin = 0.5, ClampMin = 0.001, UIMax = 10.0))
   float CableTension = 1.35f;

   /// How heavy the cable is, per-meter
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Cable Simulation", Meta = (UIMin = 0.1, ClampMin = 0.001))
   float CableMassPerMeter = 50.0f;

   /// The number of center points to add to the cable. This is essentially the number of spline points, excluding the first and last.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Cable Simulation", Meta = (UIMin = 1, ClampMin = 0, UIMax = 8))
   int32 CableNumCenterPoints = 1;

   /// When the cable physics are pre-computed, this is the number of seconds of real-world time the cable physics simulation should run before stopping.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Cable Simulation", Meta = (UIMin = 0, ClampMin = 0, UIMax = 60))
   float CableSimulationTimeSeconds = 5.0f;

   /// The number of frames per second that the cable simulation should run. This is effectively the time-step.
   /// Lower values will cause the simulation to behave in a very janky way.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Cable Simulation", Meta = (UIMin = 2, ClampMin = 1, UIMax = 60))
   int32 CableSimulationStepsPerSecond = 30;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Spline Cable Simulation")
   FTATCableSimulationSettings CableSimulationSettings;

   /// When cable simulation is run, this determines the start and end points of the simulated cable.
   /// If this returns false, the cable simulation assumes that we don't have one or more valid endpoints and will not be run.
   UFUNCTION(BlueprintPure, BlueprintNativeEvent, Category = "Power Line")
   bool GetCableSimulationStartAndEnd(FVector& startLocation, FVector& endLocation) const;

   /// Called to init auto-generated spline mesh components
   UFUNCTION(BlueprintNativeEvent, Category = "Power Line")
   void OnSetupAutoGeneratedSplineMeshComponent(USplineMeshComponent* splineMeshComponent);

   UFUNCTION(BlueprintNativeEvent, Category = "Power Line")
   void OnCharacterStartTouchingPowerLine(ACharacter* character, float nearestSplineInputKey);

   UFUNCTION(BlueprintNativeEvent, Category = "Power Line")
   void OnCharacterStopTouchingPowerLine(ACharacter* character);

   /// Gets a world location along the power line.
   /// 0.0 means the start location, 1.0 means the end location.
   UFUNCTION(BlueprintPure, Category = "Power Line")
   FVector GetPowerLineWorldLocation(float normalizedLocation) const;

   /// Gets the length of the power line in world space
   UFUNCTION(BlueprintPure, Category = "Power Line")
   float GetPowerLineLength() const;

   /// Checks if a character is currently walking on the power line
   UFUNCTION(BlueprintPure, Category = "Power Line")
   bool IsCharacterOnPowerLine(ACharacter* character) const { return character != nullptr && _charactersOnPowerLine.Contains(character); }

   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "Power Line")
   void GetAllCharactersOnPowerLine(TArray<ACharacter*>& allCharacters) const { allCharacters = _charactersOnPowerLine; }

   /// Gets a character's location as a spline input key.
   /// Returns true if the character is actually on the power line.
   /// As long as the character is valid, the returned input key will be valid whether the character is on the power line or not.
   UFUNCTION(BlueprintPure, Category = "Power Line")
   bool FindSplineInputKeyForCharacterOnPowerLine(ACharacter* character, float& splineInputKey) const;

private:
   void _AddPowerLineGameplayTags(ACharacter* character);
   void _RemovePowerLineGameplayTags(ACharacter* character);

   void _StartPowerSurge(ACharacter* character);
   void _EndPowerSurge(ACharacter* character);

   void _AuthorityApplyPowerSurge(ACharacter* character);
   void _AuthorityRemovePowerSurge(ACharacter* character);

   UFUNCTION()
   void _OnPowerSurgeChanged(bool powerSurgeEnabled);

   UPROPERTY(Transient)
   TArray<ACharacter*> _charactersOnPowerLine;

   UPROPERTY(Transient)
   TMap<ACharacter*, FActiveGameplayEffectHandle> _powerSurgeEffectHandles;

   TSet<TWeakObjectPtr<ACharacter>> _charactersAffectedByPowerSurge;

};
