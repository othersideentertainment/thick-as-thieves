// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "Player/AimAssist/OSEAimAssistComponent.h"

// ue4
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"
#include "Engine/CollisionProfile.h"

#include "TATAimAssistComponent.generated.h"

class AOSEPlayerController;

struct FOSEMantleSettings;

//---------------------------------------------------------------------------------------
// UTATAimAssistComponent
//---------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class TAT_API UTATAimAssistComponent : public UOSEAimAssistComponent
{
   GENERATED_BODY()

public:
   UTATAimAssistComponent();

   // from UActorComponent
   virtual void BeginPlay() override;

   // from UOSEAimAssistComponent
   virtual FRotator ProcessAimAssist(const AOSEPlayerController& owningPC, const FRotator& inputRot, float deltaTime) override;

private:
   void _DoInteractionAimAssist(ACharacter* character);
   void _DoMantleBasedVrilwireAimAssist(ACharacter* character, const FRotator& inputRot, float deltaTime);

   bool _TryMantleSweep(const FRotator& searchRotation, const FVector& eyesLoc, ACharacter* character, const FOSEMantleSettings& aimAssistMantleSettings);

protected:
   /// How big a radius to scan for interactables.
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Interactables")
   float InteractableDetectionDistance = 500.0f;
   
   /// Aim assist multiplier for the inner box around the interactable highlight object
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Interactables")
   float InteractableAimAssistInnerBoxMultiplier = 1.0f;
   
   /// Aim assist multiplier for the outer box around the interactable highlight object
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Interactables")
   float InteractableAimAssistOuterBoxMultiplier = 2.0f;

   /// The tool tag for the vrilwire: we only run the aim assist code if it's equipped
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   FGameplayTag VrilwireToolTag;

   /// If true, use `VrilwireMaxAngleFacingForMantleQuery` as the max facing angle for the mantle query in aim assist
   /// If false, use the max facing angle in the character's mantle settings
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   bool VrilwireOverrideMaxAngleFacingForMantleQuery = false;

   /// If `VrilwireOverrideMaxAngleFacingForMantleQuery` is true, use this as the max facing angle for the mantle query
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire", Meta = (ClampMin = "0.0", UIMin = "0.0", ClampMax = "90.0", UIMax = "90.0", EditCondition = "VrilwireOverrideMaxAngleFacingForMantleQuery", EditConditionHides))
   float VrilwireMaxAngleFacingForMantleQuery = 40.0f;

   /// The maximum distance an edge can be at for us to use it as an aim assist target
   /// Should match the rough max distance of the vrilwire tool
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   float MaxVrilwireDistance = 2000.0f;

   /// Start the mantle query this distance from the point we're looking at
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   float VrilwireMantleQueryStartingDistance = 250.0f;

   /// If an edge is closer than this, we won't treat it as an aim assist target
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   float MinVrilwireDistance = 250.0f;

   /// World-space positional offset from the floor of the ledge location when calculating the location
   /// of the aim assist target
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   FVector VrilwireEdgeTargetBias = FVector(0.0f, 0.0f, -10.0f);

   /// How many mantle searches it will do above and below the initial one (including the initial one)
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   int32 VrilwireSearchCount = 4;

   /// The pitch increment when tracing for new mantle locations
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   float VrilwirePitchSpread = 3;

   /// Whether to stop doing mantle traces after the first one had been found
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   bool VrilwireStopOnFirstTarget = true;

   /// The radius of the sphere in the initial sweep to determine what edge we're looking at
   /// Increasing this will make it easier to use aim assist on a ledge we're pointing above,
   /// but can potentially get confused by geometry partially between the player and the ledge
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   float VrilwireInitialTraceRadius = 50.0f;

   /// Increase the mantle query velocity by this amount. This is a multiplier for
   /// the distance from the query start to where we're looking: having this slightly above 1
   /// ensures that we don't fall short of the edge in the mantle query
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   float VrilwireAimAssistOverreach = 1.2f;

   /// Aim assist size multiplier for the inner box around the edge target
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   float VrilwireAimAssistInnerBoxMultiplier = 1.0f;

   /// Aim assist size multiplier for the outer box around the edge target
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   float VrilwireAimAssistOuterBoxMultiplier = 2.0f;

   /// The world-space extents used for the aim assist target for the ledge
   UPROPERTY(EditDefaultsOnly, Category = "Aim Assist|Vrilwire")
   float VrilwireEdgeTargetExtents = 50.0f;

private:
   FTraceHandle _interactTraceHandle;
};
