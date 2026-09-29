// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "AI/Alertness/AlertnessEnums.h"
#include "UI/TATWorldSpaceUserWidget.h"
#include "TATNPCDetectionVisualizerWidget.generated.h"

class UImage;
enum class EAlertnessLevel : uint8;

UCLASS()
class TAT_API UTATNPCDetectionVisualizerWidget : public UTATWorldSpaceUserWidget
{
   GENERATED_BODY()
   
public:
   UPROPERTY(EditDefaultsOnly, meta=(BindWidget), BlueprintReadOnly)
   UImage* OffscreenArrow { nullptr };
   
   UPROPERTY(EditDefaultsOnly, meta=(BindWidget), BlueprintReadOnly)
   UImage* DetectionMeter { nullptr };

   virtual void NativeOnInitialized() override;
   virtual void TickScreenPosition_Implementation() override;
   virtual void OnWidgetClampStateChanged_Implementation(bool bIsClamped) override;
   
protected:
   UPROPERTY(EditDefaultsOnly, Category="Setup")
   float _MaxDistanceToDisplay { 6000.f};
   bool _IsDetectionMeterCurrentlyVisible { false };
   bool _IsDetectionMeterCurrentlyOffScreen { false };
   bool _PendingDetectionMeterVisibility { false };

   UPROPERTY(EditDefaultsOnly, Category="Setup")
   TMap<EAlertnessLevel, FLinearColor> _AlertnessColors;
   
   UPROPERTY(EditDefaultsOnly, Category="Setup")
   float _AnimationPlaybackSpeed { 1.f };

   EAlertnessLevel _PreviousAlertnessLevel { EAlertnessLevel::Neutral };
   
   UFUNCTION()
   void _HandleWidgetDisplayAnimationComplete(bool bOutForward);
   void _HandleHideWidget();
   void _TryDisplayWidget(EAlertnessLevel alertnessLevel, bool shouldShow);
   
   UWidgetAnimation* _GetAnimationForAlertnessLevel(EAlertnessLevel alertnessLevel, const bool isIn);
   
   UFUNCTION(BlueprintNativeEvent)
   UWidgetAnimation* _GetAnimationForAlertnessLevel(EAlertnessLevel alertnessLevel, bool isIn, bool isOffScreen);
   UUMGSequencePlayer* _HandlePlayingAnimationForAlertnessLevel(const EAlertnessLevel& toAlertLevel, bool isIn);
   void _PlayAlertTransitionAnimations(const EAlertnessLevel& fromAlertLevel, const EAlertnessLevel& toAlertLevel);
   void _SetupVisuals(float normalizedDetectionLevel,
                      const EAlertnessLevel& alertnessLevel);   
};
