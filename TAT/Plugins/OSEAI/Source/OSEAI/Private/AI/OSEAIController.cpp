// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/OSEAIController.h"

// ose
#include "AI/OSEKnowledgeComponent.h"
#include "AI/Perception/OSEAIPerceptionComponent.h"
#include "AI/Utility/UtilityAIBehaviorComponent.h"
#include "AI/Utility/UtilityAIGoalComponent.h"
#include "Character/OSECharacterBase.h"
#include "Projectiles/OSEProjectileFunctionLibrary.h"
#include "Abilities/OSEAbilitySystemComponent.h"

// ue4
#include "AbilitySystemGlobals.h"
#include "Kismet/GameplayStatics.h"
#include "NavigationSystem.h"
#include "NavigationSystemTypes.h"
#include "VisualLogger/VisualLogger.h"
#include "VisualLogger/VisualLoggerTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAIController)

#if WITH_RECAST
#include "NavMesh/RecastNavMesh.h"
#endif // WITH_RECAST

DEFINE_LOG_CATEGORY(LogOSEAIController);

namespace OSEAIControllerCVars
{
   static int ClampControlRotation = 1;
   FAutoConsoleVariableRef CVarDebugCombatHitboxes(
      TEXT("OSE.AI.ClampControlRotation"),
      ClampControlRotation,
      TEXT("Clamp the AI control rotation?"),
      ECVF_Default);
}

AOSEAIController::AOSEAIController(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
   , _interpSpeedBody(2)
   , _interpSpeedHead(5)
{
   bAttachToPawn = true;
   bAllowStrafe = true;
   bSetControlRotationFromPawnOrientation = true;
}

// This is the pawn (BODY) rotation to use. It only modifies the pawn rotation 
// if the movement component has bUseControllerDesiredRotation set to true.
// Overriding so we can return a smoothed rotation.
FRotator AOSEAIController::GetDesiredRotation() const
{
   return _smoothedBodyRotation;
}


// Overriding so we can update smoothed rotations
void AOSEAIController::UpdateControlRotation(float deltaTime, bool updatePawn /*= true*/)
{
   // NOTE:  Some focus-related code copied from AAIController::UpdateControlRotation so we can customize some behavior.
   if (APawn* const myPawn = GetPawn())
   {
      FRotator targetControlRotation = GetControlRotation();

      // Look toward focus
      const FVector focalPoint = GetFocalPoint();
      if (FAISystem::IsValidLocation(focalPoint))
      {
         targetControlRotation = (focalPoint - myPawn->GetPawnViewLocation()).Rotation();
      }
      else if (bSetControlRotationFromPawnOrientation)
      {
         targetControlRotation = myPawn->GetActorRotation();
      }

      // TODO: Separate head/body rotations
      _rawBodyRotation = targetControlRotation;
      _rawHeadRotation = targetControlRotation;
      
      {
         // clamp control rotation pitch so we can't look up/down into our stomach or above our heads (similar to APlayerCameraManager::ProcessViewRotation)
         if (OSEAIControllerCVars::ClampControlRotation)
         {
            _rawHeadRotation.Pitch = FMath::ClampAngle(_rawHeadRotation.Pitch, _headRotationPitchMin, _headRotationPitchMax);
            _rawHeadRotation.Pitch = FRotator::ClampAxis(_rawHeadRotation.Pitch);
         }
      }

      // Smooth desired (pawn/body) rotation
      {
         _smoothedBodyRotation = FMath::RInterpTo(_smoothedBodyRotation, _rawBodyRotation, deltaTime, GetInterpSpeedBody());
      }

      // Smooth control (head) rotation
      {
         _smoothedHeadRotation = FMath::RInterpTo(_smoothedHeadRotation, _rawHeadRotation, deltaTime, GetInterpSpeedHead());
      }
      
    
      if(_forcedRotation.IsSet())
      {
         _smoothedHeadRotation = myPawn->GetTransform().TransformRotation(_forcedRotation.GetValue().GetInverse().Quaternion()).Rotator();
      }
      
      // control rotation will be the head / look-at rotation
      SetControlRotation(_smoothedHeadRotation);

      if (updatePawn)
      {
         const FRotator currentPawnRotation = myPawn->GetActorRotation();

         if (!currentPawnRotation.Equals(GetControlRotation(), 1e-3f))
         {
            myPawn->FaceRotation(GetControlRotation(), deltaTime);
         }
      }
   }
}

// Overriding so we don't grab actor location but where we think the actor is
FVector AOSEAIController::GetFocalPointOnActor(const AActor* targetActor) const
{
   FVector returnPoint;

   if (auto targetPawn = Cast<APawn>(targetActor))
   {
      returnPoint = targetPawn->GetPawnViewLocation();
   }
   else
   {
      returnPoint = Super::GetFocalPointOnActor(targetActor);
   }

   // If we aren't configured to lead our target with an intercept path, we're done.
   float myInterceptSpeed = _interceptSpeed;
   if (myInterceptSpeed<SMALL_NUMBER)
   {
      return returnPoint;
   }

   // Find an intercept course on (presumably) moving actor
   FVector targetVelocity = targetActor->GetVelocity();

   // Add uncertainty (if any) to velocity
   if (_targetVelocityUncertainty > 0.0f)
   {
      const float timeSeconds = GetWorld()->GetTimeSeconds();
      const float targetVelocityUncertainty = _targetVelocityUncertainty * FMath::PerlinNoise1D(timeSeconds);
      targetVelocity = targetVelocity + (targetVelocity * targetVelocityUncertainty);
   }

   FVector myPawnEyeLoc;
   FRotator unusedRot;

   GetPlayerViewPoint(myPawnEyeLoc, unusedRot);

   // note that _interceptGravity is the acceleration on the _projectile_, and PlotInterceptTime wants the
   // acceleration on the _target_. But, hey, there's no universally preferred frame of reference,
   // so we can just reverse the sign.
   float interceptTime = UOSEProjectileFunctionLibrary::PlotInterceptTime(myPawnEyeLoc, myInterceptSpeed, returnPoint, targetVelocity, -_interceptGravity);

   // No intercept course possible, or intercept is trivial? Just dead reckon at target.
   if (FMath::IsNaN(interceptTime) || interceptTime<SMALL_NUMBER)
   {
      return returnPoint;
   }

   returnPoint += interceptTime * targetVelocity;
   FVector fall = 0.5f * _interceptGravity * interceptTime * interceptTime;
   returnPoint -= fall;

   return returnPoint;
}

// Overriding so we can reset smoothed rotations
void AOSEAIController::PostInitializeComponents()
{
   const FRotator rawControlRotation = Super::GetControlRotation();
   _rawBodyRotation = rawControlRotation;
   _rawHeadRotation = rawControlRotation;
   _smoothedBodyRotation = rawControlRotation;
   _smoothedHeadRotation = rawControlRotation;

   Super::PostInitializeComponents();

   if (!_cachedUtilityAIBehaviorComponent)
   {
      _cachedUtilityAIBehaviorComponent = FindComponentByClass<UUtilityAIBehaviorComponent>();
   }

   if (!_cachedUtilityAIGoalComponent)
   {
      _cachedUtilityAIGoalComponent = FindComponentByClass<UUtilityAIGoalComponent>();
   }
   
   if (!_cachedOSEPerceptionComponent)
   {
      _cachedOSEPerceptionComponent = FindComponentByClass<UOSEAIPerceptionComponent>();
   }

   if (!_cachedKnowledgeComponent)
   {
      _cachedKnowledgeComponent = FindComponentByClass<UOSEKnowledgeComponent>();
   }
}

void AOSEAIController::SetPawn(APawn* inPawn)
{
   Super::SetPawn(inPawn);

   _oseCharacter = Cast<AOSECharacterBase>(GetPawn());
}

void AOSEAIController::OnBehaviorTreeFinished()
{
   if (_cachedUtilityAIBehaviorComponent)
   {
      _cachedUtilityAIBehaviorComponent->OnBehaviorTreeFinished();
   }
}

// Overriding so we can reset smoothed rotations
void AOSEAIController::OnPossess(APawn* inPawn)
{
   Super::OnPossess(inPawn);

   const FRotator rawControlRotation = Super::GetControlRotation();
   _rawBodyRotation = rawControlRotation;
   _rawHeadRotation = rawControlRotation;
   _smoothedBodyRotation = rawControlRotation;
   _smoothedHeadRotation = rawControlRotation;

   OnPossessedPawn.Broadcast(inPawn);
}

void AOSEAIController::OnUnPossess()
{
   Super::OnUnPossess();

   OnUnPossessedPawn.Broadcast();
}

#if ENABLE_VISUAL_LOG
void AOSEAIController::GrabDebugSnapshot(FVisualLogEntry* snapshot) const
{
   Super::GrabDebugSnapshot(snapshot);

   if (_cachedUtilityAIBehaviorComponent)
   {
      _cachedUtilityAIBehaviorComponent->DescribeSelfToVisLog(snapshot);
   }

   if (_cachedUtilityAIGoalComponent)
   {
      _cachedUtilityAIGoalComponent->DescribeSelfToVisLog(snapshot);
   }
}
#endif

UOSEAlertnessComponent* AOSEAIController::GetAlertnessComponent() const
{
   // pass-through
   if (const IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(GetPawn()))
   {
      return alertnessInterface->GetAlertnessComponent();
   }
   return nullptr;
}

EAlertnessLevel AOSEAIController::GetAlertnessLevel() const
{
   // pass-through
   if (const IOSEAlertnessInterface* alertnessInterface = Cast<IOSEAlertnessInterface>(GetPawn()))
   {
      return alertnessInterface->GetAlertnessLevel();
   }
   return EAlertnessLevel::Neutral;
}

void AOSEAIController::SetForcedControlRotation(const FRotator& rotator)
{
   _forcedRotation = rotator;
}

void AOSEAIController::ClearForcedControlRotation()
{
   _forcedRotation.Reset();
}

