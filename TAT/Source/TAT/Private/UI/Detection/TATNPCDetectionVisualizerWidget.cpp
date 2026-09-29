// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "UI/Detection/TATNPCDetectionVisualizerWidget.h"

// TAT
#include "Character/TATTeams.h"

// OSE 
#include "AI/Alertness/AlertnessEnums.h"
#include "AI/Alertness/DetectionEnums.h"
#include "AI/Alertness/OSEAlertnessInterface.h"
#include "Character/OSETeamInterface.h"
#include "Detection/OSEDetectionComponent.h"
#include "Detection/OSEDetectionComponentInterface.h"
#include "OSEProjectSettings.h"

// UE
#include "GameplayTagAssetInterface.h"
#include "Animation/UMGSequencePlayer.h"
#include "Components/Image.h"
#include "Kismet/GameplayStatics.h"
#include "UI/AsyncTaskAnimateWidget.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATNPCDetectionVisualizerWidget)

DECLARE_CYCLE_STAT(TEXT("NPC Detection: TickScreenPosition"), STAT_TAT_NPC_DETECTION_WIDGET_TICK, STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("NPC Detection: Alertness Interface Gather"), STAT_TAT_NPC_DETECTION_WIDGET_ALERTNESSCOMPONENT, STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("NPC Detection: Team"), STAT_TAT_NPC_DETECTION_WIDGET_TEAM, STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("NPC Detection: Detection Component"), STAT_TAT_NPC_DETECTION_WIDGET_DETECTION_COMPONENT, STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("NPC Detection: Gameplay Tag"), STAT_TAT_NPC_DETECTION_WIDGET_GAMEPLAY_TAG, STATGROUP_UI);
DECLARE_CYCLE_STAT(TEXT("NPC Detection: Setup Visuals"), STAT_TAT_NPC_DETECTION_WIDGET_SETUP_VISUALS, STATGROUP_UI);


void UTATNPCDetectionVisualizerWidget::NativeOnInitialized()
{
   Super::NativeOnInitialized();
}

void UTATNPCDetectionVisualizerWidget::TickScreenPosition_Implementation()
{
   SCOPE_CYCLE_COUNTER(STAT_TAT_NPC_DETECTION_WIDGET_TICK);
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATNPCDetectionVisualizerWidget::TickScreenPosition_Implementation)
   const APawn* localPlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
   
   if (localPlayerPawn == nullptr || _sceneComponentToTrack == nullptr)
      return;
   
   AActor* trackedActor = _sceneComponentToTrack->GetOwner();
   if (trackedActor == nullptr)
      return;
   
   auto _AttemptToHideWidgetAndTickPositionIfNeeded = [this](const EAlertnessLevel alertnessLevel)
   {
      _TryDisplayWidget(alertnessLevel, false);
      if (_IsDetectionMeterCurrentlyVisible)
      {
         // if we're trying to play an animation going out of range, make sure we tick just long enough to stay above the target
         Super::TickScreenPosition_Implementation();
      }
   };
   
   const float distanceToTarget = localPlayerPawn->GetDistanceTo(trackedActor);
   // if we're too far away, don't display.
   if (distanceToTarget > _MaxDistanceToDisplay)
   {
      _AttemptToHideWidgetAndTickPositionIfNeeded(_PreviousAlertnessLevel);
      return;
   }
      
   EAlertnessLevel currentAlertnessLevel = EAlertnessLevel::Neutral; 
   {
      SCOPE_CYCLE_COUNTER(STAT_TAT_NPC_DETECTION_WIDGET_ALERTNESSCOMPONENT);
      TRACE_CPUPROFILER_EVENT_SCOPE(UTATNPCDetectionVisualizerWidget::TickScreenPosition_Implementation_Alertness)

      if (const IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(trackedActor))
      {
         if (const UOSEAlertnessComponent* alertnessComponent = alertnessInterface->GetAlertnessComponent())
         {
            currentAlertnessLevel = alertnessComponent->GetAlertnessLevel();
         }
      }
   }
   
   float normalizedDetectionValue = 0.f;
   EActorDetectionState detectionState = EActorDetectionState::Observing;
   {
      SCOPE_CYCLE_COUNTER(STAT_TAT_NPC_DETECTION_WIDGET_TEAM);
      TRACE_CPUPROFILER_EVENT_SCOPE(UTATNPCDetectionVisualizerWidget::TickScreenPosition_Implementation_Team)
      
      const EOSETeamAttitude teamAttitude = UTATTeamAttitudeSolver::GetTeamAttitudeBetweenActorsWithDisguise(
          trackedActor,
          localPlayerPawn,
          ETATTeamDisguiseHandling::UseApparentTeam
       );
   
      // if we aren't hostile, we don't display the detection meter.
      if (teamAttitude != EOSETeamAttitude::Hostile)
      {
         _AttemptToHideWidgetAndTickPositionIfNeeded(_PreviousAlertnessLevel);
         return;
      }
   }
   {
      SCOPE_CYCLE_COUNTER(STAT_TAT_NPC_DETECTION_WIDGET_DETECTION_COMPONENT);
      TRACE_CPUPROFILER_EVENT_SCOPE(UTATNPCDetectionVisualizerWidget::TickScreenPosition_Implementation_Detection)
      
      if (const IOSEDetectionComponentInterface* detectionComponentInterface = Cast<IOSEDetectionComponentInterface>(trackedActor))
      {
         if (const auto detectionComponent = detectionComponentInterface->GetDetectionComponent())
         {
            normalizedDetectionValue = detectionComponent->GetDetectionValueForPlayer(localPlayerPawn);
            detectionState = detectionComponent->GetDetectionStateForPlayer(localPlayerPawn);
         }
      }
   }
   
   bool isUnconscious = false;
   {
      SCOPE_CYCLE_COUNTER(STAT_TAT_NPC_DETECTION_WIDGET_GAMEPLAY_TAG);
      TRACE_CPUPROFILER_EVENT_SCOPE(UTATNPCDetectionVisualizerWidget::TickScreenPosition_Implementation_GameplayTag)
      
      if (const IGameplayTagAssetInterface* assetInterface = Cast<IGameplayTagAssetInterface>(trackedActor))
      {
         isUnconscious = assetInterface->HasMatchingGameplayTag(UOSEProjectSettings::Get().ConditionUnconsciousTag);
      }
   }
   
   const bool isObserving = detectionState == EActorDetectionState::Observing || normalizedDetectionValue <= 0.f;
   if ((currentAlertnessLevel == EAlertnessLevel::Neutral && isObserving) || isUnconscious)
   {
      _AttemptToHideWidgetAndTickPositionIfNeeded(currentAlertnessLevel);
      return;
   }
   
   _TryDisplayWidget(currentAlertnessLevel, true);
   _SetupVisuals(normalizedDetectionValue, currentAlertnessLevel);
   
   // update widget position
   Super::TickScreenPosition_Implementation();
   
}

void UTATNPCDetectionVisualizerWidget::OnWidgetClampStateChanged_Implementation(bool bIsClamped)
{
   Super::OnWidgetClampStateChanged_Implementation(bIsClamped);
   
}

void UTATNPCDetectionVisualizerWidget::_HandleWidgetDisplayAnimationComplete(bool bOutForward)
{
   _HandleHideWidget();
}

void UTATNPCDetectionVisualizerWidget::_HandleHideWidget()
{
   _IsDetectionMeterCurrentlyVisible = false;
   SetVisibility(ESlateVisibility::Collapsed);
}

void UTATNPCDetectionVisualizerWidget::_TryDisplayWidget(const EAlertnessLevel alertnessLevel, const bool shouldShow)
{
   if (shouldShow == _PendingDetectionMeterVisibility && IsWidgetClamped() == _IsDetectionMeterCurrentlyOffScreen)
      return;
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATNPCDetectionVisualizerWidget::_TryDisplayWidget)
   if (shouldShow)
   {
      SetVisibility(ESlateVisibility::HitTestInvisible);
   }
   _PendingDetectionMeterVisibility = shouldShow;
   _IsDetectionMeterCurrentlyOffScreen = IsWidgetClamped();
   UWidgetAnimation* animToPlay = _GetAnimationForAlertnessLevel(alertnessLevel, shouldShow);
   if (animToPlay == nullptr)
   {
      _PendingDetectionMeterVisibility = shouldShow;
      _IsDetectionMeterCurrentlyVisible = shouldShow;
      if (shouldShow == false)
      {
         _HandleHideWidget();
      }
      return;
   }
   
   UAsyncTaskAnimateWidget* animTask = UAsyncTaskAnimateWidget::SpawnAsyncAnimateWidgetTask(
      this,
      animToPlay,
      0.f,
      1,
      _AnimationPlaybackSpeed,
      false,
      shouldShow);

   if (animTask && shouldShow == false)
   {
      animTask->Completed.AddUniqueDynamic(this, &ThisClass::_HandleWidgetDisplayAnimationComplete);
   }
}

UWidgetAnimation* UTATNPCDetectionVisualizerWidget::_GetAnimationForAlertnessLevel(
   const EAlertnessLevel alertnessLevel, 
   const bool isIn)
{
   const bool isOffscreen = IsWidgetClamped();
   return _GetAnimationForAlertnessLevel(alertnessLevel, isIn, isOffscreen);
}

UWidgetAnimation* UTATNPCDetectionVisualizerWidget::_GetAnimationForAlertnessLevel_Implementation(
   EAlertnessLevel alertnessLevel,
   bool isIn,
   bool isOffScreen)
{
   return nullptr;
}

UUMGSequencePlayer* UTATNPCDetectionVisualizerWidget::_HandlePlayingAnimationForAlertnessLevel(
   const EAlertnessLevel& toAlertLevel,
   const bool isIn)
{
   if (UWidgetAnimation* const animToPlay = _GetAnimationForAlertnessLevel(toAlertLevel, isIn))
   {
      return PlayAnimation(animToPlay, 0.f, 1, EUMGSequencePlayMode::Forward, _AnimationPlaybackSpeed);
   }
   return nullptr;
}

void UTATNPCDetectionVisualizerWidget::_PlayAlertTransitionAnimations(
   const EAlertnessLevel& fromAlertLevel,
   const EAlertnessLevel& toAlertLevel)
{
   UUMGSequencePlayer* animPlayer = _HandlePlayingAnimationForAlertnessLevel(fromAlertLevel, false);
   if (animPlayer == nullptr)
   {
      _HandlePlayingAnimationForAlertnessLevel(toAlertLevel, true);
      return;
   }
   animPlayer->OnSequenceFinishedPlaying().AddLambda([this, toAlertLevel](UUMGSequencePlayer& player)
   {
      _HandlePlayingAnimationForAlertnessLevel(toAlertLevel, true);
   });
}

void UTATNPCDetectionVisualizerWidget::_SetupVisuals(const float normalizedDetectionLevel,
                                                     const EAlertnessLevel& alertnessLevel)
{
   SCOPE_CYCLE_COUNTER(STAT_TAT_NPC_DETECTION_WIDGET_SETUP_VISUALS);
   TRACE_CPUPROFILER_EVENT_SCOPE(UTATNPCDetectionVisualizerWidget::_SetupVisuals)
   UMaterialInstanceDynamic* offscreenDynamicMaterial = OffscreenArrow->GetDynamicMaterial();
   UMaterialInstanceDynamic* detectionMeterDynamicMaterial = DetectionMeter->GetDynamicMaterial();

   FLinearColor colorToUse = FColor::Black;
   if (_AlertnessColors.Contains(alertnessLevel))
   {
      colorToUse = _AlertnessColors[alertnessLevel];
   }
   static const FName colorName = FName(TEXT("Color"));
   static const FName percentageName = FName(TEXT("Percent"));
   offscreenDynamicMaterial->SetVectorParameterValue(colorName, colorToUse);
   detectionMeterDynamicMaterial->SetScalarParameterValue(percentageName, normalizedDetectionLevel);
   
   if (alertnessLevel != _PreviousAlertnessLevel)
   {
      _PlayAlertTransitionAnimations(_PreviousAlertnessLevel, alertnessLevel);
      _PreviousAlertnessLevel = alertnessLevel;
   }
}
