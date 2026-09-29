// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#include "AI/TATAIStateWorldSubsystem.h"

// tat
#include "AI/TATAISettings.h"
#include "Character/TATCharacterAIBase.h"

// ue
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PlayerState.h"
#include "DrawDebugHelpers.h"
#include "Detection/OSEDetectionComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIStateWorldSubsystem)

namespace AIStateWorldSubsystemCVars
{
   static float UpdateTimer = 1.0f;
   FAutoConsoleVariableRef CVarAIStateUpdateTimer(
      TEXT("TAT.AIState.UpdateTimer"),
      UpdateTimer,
      TEXT("How often do we update the various max AI states?"),
      ECVF_Default);

   static int32 DrawDebug = 0;
   FAutoConsoleVariableRef CVarDrawDebug(
      TEXT("TAT.AIState.DrawDebug"),
      DrawDebug,
      TEXT("Draw debug visualization for ranges?"),
      ECVF_Default);
}

void UTATAIStateWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
   Super::Initialize(Collection);

   UWorld* world = GetWorld();
   check(world);

   if (!world->IsNetMode(NM_DedicatedServer))
   {
      world->GetTimerManager().SetTimer(_updateAIStateMaxSeenTimerHandle, this, &UTATAIStateWorldSubsystem::_UpdateAIStateMaxSeen, AIStateWorldSubsystemCVars::UpdateTimer, true);
   }
}
 
void UTATAIStateWorldSubsystem::Deinitialize()
{
   Super::Deinitialize();

   UWorld* world = GetWorld();
   check(world);
   world->GetTimerManager().ClearTimer(_updateAIStateMaxSeenTimerHandle);
}

void UTATAIStateWorldSubsystem::RegisterAICharacter(const ATATCharacterAIBase* character)
{
   check(character);
   _registeredAI.Emplace(character);
}

void UTATAIStateWorldSubsystem::OnAIAlertnessLevelChanged(const ATATCharacterAIBase* character)
{
   _UpdateAIStateMaxSeen();
}

void UTATAIStateWorldSubsystem::SetLocalPlayerIsUsingMonocular(bool isUsingMonocular)
{
   if (isUsingMonocular != _isLocalPlayerCurrentlyUsingMonocular)
   {
      _isLocalPlayerCurrentlyUsingMonocular = isUsingMonocular;
      OnLocalPlayerMonocularUsageChanged.Broadcast(_isLocalPlayerCurrentlyUsingMonocular);
   }
}

void UTATAIStateWorldSubsystem::SetLocalPlayerIsStandingStill(const bool isStandingStill)
{
   if (_isLocalPlayerStandingStill != isStandingStill)
   {
      _isLocalPlayerStandingStill = isStandingStill;
      OnLocalPlayerIsStandingStillChanged.Broadcast(isStandingStill);
   }
}

void UTATAIStateWorldSubsystem::_UpdateAIStateMaxSeen()
{
   UWorld* world = GetWorld();
   check(world);

   AGameStateBase* gameState = world->GetGameState();
   APlayerController* controller = world->GetFirstPlayerController();
   APawn* localPawn = controller ? controller->GetPawn() : nullptr;
   if (!localPawn || !gameState)
      return;

   const EAlertnessLevel prevMaxAlertness = _maxAlertnessSeen;
   const EActorDetectionState prevMaxDetectionStateSeen = _maxDetectionStateSeen;
   const float prevMaxDetectionValueSeen = _maxDetectionValueSeen;

   _maxAlertnessSeen = EAlertnessLevel::Neutral;
   _maxDetectionStateSeen = EActorDetectionState::Observing;
   _maxDetectionValueSeen = 0.0f;

   const UTATAISettings& settings = UTATAISettings::Get();
   float enterMaxDistanceSquared = settings.DistanceForMaxAlertnessCalculation * settings.DistanceForMaxAlertnessCalculation;
   float exitMaxDistanceSquared = FMath::Square(settings.DistanceForMaxAlertnessCalculation + settings.ExtraDistanceForMaxAlertnessCalculationBuffer);

   const ATATCharacterAIBase* maxAlertnessIncreasedByAI = nullptr;

#if ENABLE_DRAW_DEBUG
   if (AIStateWorldSubsystemCVars::DrawDebug)
   {
      DrawDebugSphere(world, localPawn->GetActorLocation(), settings.DistanceForMaxAlertnessCalculation, 12, FColor::Purple, false, AIStateWorldSubsystemCVars::UpdateTimer);
      DrawDebugSphere(world, localPawn->GetActorLocation(), settings.DistanceForMaxAlertnessCalculation + settings.ExtraDistanceForMaxAlertnessCalculationBuffer, 12, FColor::Orange, false, AIStateWorldSubsystemCVars::UpdateTimer);
   }
#endif

   _registeredAI.RemoveAll([&](FRegisteredAIState& entry)
   {
      const ATATCharacterAIBase* trackedCharacter = entry.Character.Get();
      // null/destroyed character, remove it.
      if (!IsValid(trackedCharacter))
         return true;

      const UOSEDetectionComponent* trackedCharacterDetectionComp = trackedCharacter->GetDetectionComponent();
      check(trackedCharacterDetectionComp);

      // don't remove it, but also don't update the max's based on it
      const float distanceSquared = localPawn->GetSquaredDistanceTo(trackedCharacter);
      if (distanceSquared > exitMaxDistanceSquared)
      {
         entry.ResetFlags();
         return false;
      }

      const bool inEnterDistance = distanceSquared <= enterMaxDistanceSquared;

      if (entry.HadAlertness || inEnterDistance)
      {
         // alertness can just get the max of all the AI
         const EAlertnessLevel aiAlertnessLevel = trackedCharacter->GetAlertnessLevel();
         entry.HadAlertness = aiAlertnessLevel != EAlertnessLevel::Neutral;

         if (aiAlertnessLevel > _maxAlertnessSeen)
         {
            _maxAlertnessSeen = aiAlertnessLevel;
            maxAlertnessIncreasedByAI = trackedCharacter;
         }
      }

      // but detection needs to check each AI's detection against all players to come up w/ a max
      if (entry.HadDetection || inEnterDistance)
      {
         entry.HadDetection = false;

         const EActorDetectionState playerDetectionState = trackedCharacterDetectionComp->GetDetectionStateForPlayer(localPawn);
         const float playerDetectionValue = trackedCharacterDetectionComp->GetDetectionValueForPlayer(localPawn);

         entry.HadDetection = entry.HadDetection || playerDetectionValue > 0;
         _maxDetectionValueSeen = FMath::Max(_maxDetectionValueSeen, playerDetectionValue);

         if (playerDetectionState > _maxDetectionStateSeen)
         {
            _maxDetectionStateSeen = playerDetectionState;
         }
      }

      return false;
   });

   // alertness
   if (_maxAlertnessSeen > prevMaxAlertness)
   {
      ensure(maxAlertnessIncreasedByAI != nullptr);
      OnMaxAlertnessIncreased.Broadcast(_maxAlertnessSeen, maxAlertnessIncreasedByAI);
   }
   else if (_maxAlertnessSeen < prevMaxAlertness)
   {
      OnMaxAlertnessDecreased.Broadcast(_maxAlertnessSeen);
   }

   // detection state
   if (_maxDetectionStateSeen > prevMaxDetectionStateSeen)
   {
      OnMaxDetectionStateIncreased.Broadcast(_maxDetectionStateSeen);
   }
   else if (_maxDetectionStateSeen < prevMaxDetectionStateSeen)
   {
      OnMaxDetectionStateDecreased.Broadcast(_maxDetectionStateSeen);
   }

   // detection value
   if (_maxDetectionValueSeen > prevMaxDetectionValueSeen)
   {
      OnMaxDetectionValueIncreased.Broadcast(_maxDetectionValueSeen);
   }
   else if (_maxDetectionValueSeen < prevMaxDetectionValueSeen)
   {
      OnMaxDetectionValueDecreased.Broadcast(_maxDetectionValueSeen);
   }
}

