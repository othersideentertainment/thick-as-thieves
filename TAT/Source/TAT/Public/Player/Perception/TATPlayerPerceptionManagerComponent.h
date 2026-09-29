// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"

// tat
#include "Player/Perception/TATPlayerPerceivableComponent.h"

#include "TATPlayerPerceptionManagerComponent.generated.h"

class UTATPlayerPerceivableComponent;

UENUM()
enum class ETATPerceivableScreenFocusRegion : uint8
{
   Offscreen,
   PeripheralVision,
   GeneralAwareness,
   FocusArea,
   Count UMETA(Hidden)
};


USTRUCT()
struct FTATPlayerPerceptionPerLevelConfig
{
   GENERATED_BODY()
public:
   UPROPERTY(EditDefaultsOnly)
   ETATPerceivableScreenFocusRegion RegionRequiredToProgress = ETATPerceivableScreenFocusRegion::PeripheralVision;

   UPROPERTY(EditDefaultsOnly)
   ETATPerceivableScreenFocusRegion RegionRequiredToNotRegress = ETATPerceivableScreenFocusRegion::PeripheralVision;

   UPROPERTY(EditDefaultsOnly, Meta = (UIMin = "0.0", ClampMin = "0.0"))
   float ProgressSpeedCoefficient = 1.0f;

   UPROPERTY(EditDefaultsOnly, Meta = (UIMin = "0.0", ClampMin = "0.0"))
   float OffscreenRegressionSpeed = 1.0f;

   UPROPERTY(EditDefaultsOnly, Meta = (UIMin = "0.0", ClampMin = "0.0"))
   float OnscreenNotVisibleRegressionSpeed = 1.0f;
};

UCLASS()
class TAT_API UTATPlayerPerceptionManagerComponent : public UActorComponent
{
   GENERATED_BODY()
public:

   UTATPlayerPerceptionManagerComponent();

   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type reason) override;
   virtual void TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction) override;

   UPROPERTY(EditDefaultsOnly, Category = "Perception System", Meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
   float GeneralAwarenessScreenPercentage = 0.7f;

   UPROPERTY(EditDefaultsOnly, Category = "Perception System", Meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
   float FocusAreaScreenPercentage = 0.3f;

   /// How much overlap in screenspace between a focus region (e.g. General Awareness or Focus Area) and the object's bounds
   /// to consider it being in that focus region
   UPROPERTY(EditDefaultsOnly, Category = "Perception System", AdvancedDisplay, Meta = (UIMin = "0.0", ClampMin = "0.0", UIMax = "1.0", ClampMax = "1.0"))
   float RequiredScreenOverlapForFocusRegion = 0.001f;

   UPROPERTY(EditDefaultsOnly, Category = "Perception System")
   FTATPlayerPerceptionPerLevelConfig NoneConfig;

   UPROPERTY(EditDefaultsOnly, Category = "Perception System")
   FTATPlayerPerceptionPerLevelConfig PerceivedConfig;

   UPROPERTY(EditDefaultsOnly, Category = "Perception System")
   FTATPlayerPerceptionPerLevelConfig AwareConfig;

   UPROPERTY(EditDefaultsOnly, Category = "Perception System")
   FTATPlayerPerceptionPerLevelConfig FocusedConfig;

   UPROPERTY(EditDefaultsOnly, Category = "Perception System")
   FTATPlayerPerceptionPerLevelConfig FixatedConfig;

private:
   struct FPerceivableFocusStatus
   {
      ETATPerceivableScreenFocusRegion Region = ETATPerceivableScreenFocusRegion::Offscreen;
      float ScreenAreaPercentage = 0.0f;
      float MaxForwardDotProduct = 0.0f;

      FPerceivableFocusStatus(ETATPerceivableScreenFocusRegion region, float screenAreaPercentage, float maxForwardDotProduct)
      {
         Region = region;
         ScreenAreaPercentage = screenAreaPercentage;
         MaxForwardDotProduct = maxForwardDotProduct;
      }
   };

   struct FPerceivableAndStatus
   {
      TWeakObjectPtr<UTATPlayerPerceivableComponent> Perceivable;
      ETATPlayerPerceptionLevel PerceptionLevel = ETATPlayerPerceptionLevel::None;
      float ProgressToNextState = 0.0f; // Can go up or down: if it reaches -1.0, regress to previous state. If it reaches 1.0, progress to next state

      void UpdateWithProgressDelta(float progressDelta);
   };

   void _TickPerceivables(float deltaTime, APlayerController* playerController, APawn* playerPawn);
   bool _PerformLOSCheck(AActor* playerPawn, FVector startingLocation, UTATPlayerPerceivableComponent* perceivable) const;
   FPerceivableFocusStatus _GetPerceivableScreenFocusArea(const FMatrix& viewProjectionMatrix, const FIntRect& constrainedViewRect, FVector eyesLocation, FVector eyesDirection, UTATPlayerPerceivableComponent* perceivable) const;
   float _GetPerceptionProgressDeltaForPerceivable(const FPerceivableAndStatus& perceivableAndStatus, const FPerceivableFocusStatus& focusState) const;
   const FTATPlayerPerceptionPerLevelConfig& _GetConfigForLevel(ETATPlayerPerceptionLevel perceptionLevel) const;

   void _OnPerceivableRegistered(UTATPlayerPerceivableComponent* perceivable);

   TArray<FPerceivableAndStatus> _perceivablesAndStatus;
};

