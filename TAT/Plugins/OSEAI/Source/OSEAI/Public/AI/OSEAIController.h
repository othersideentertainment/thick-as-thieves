// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Alertness/AlertnessEnums.h"
#include "AI/Alertness/OSEAlertnessInterface.h"

// ue
#include "AIController.h"
#include "GameplayTagContainer.h"
#include "OSEAIController.generated.h"

class AOSECharacterBase;
class UOSEKnowledgeComponent;
class UOSEAIPerceptionComponent;
class UUtilityAIBehaviorComponent;
class UUtilityAIGoalComponent;
#if ENABLE_VISUAL_LOG
struct FVisualLogEntry;
#endif

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnPossessedPawn, APawn*, pawn);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnUnPossessedPawn);

DECLARE_LOG_CATEGORY_EXTERN(LogOSEAIController, Log, All);

/// OSE-specific override of AI Controller
UCLASS(ClassGroup = AI, BlueprintType, Blueprintable)
class OSEAI_API AOSEAIController 
   : public AAIController
   , public IOSEAlertnessInterface
{
   GENERATED_BODY()

public:
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FOnStimReceived, AActor*, actor, EStimSeverity, stimSeverity, FName, stimTag);

public:

   AOSEAIController(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   /// Overriding so we can reset smoothed rotations
   virtual void PostInitializeComponents() override;

   virtual void SetPawn(APawn* inPawn) override;

   /// This is the pawn (BODY) rotation to use. It only modifies the pawn rotation
   /// if the movement component has bUseControllerDesiredRotation set to true.
   /// Overriding so we can return a smoothed rotation.
   virtual FRotator GetDesiredRotation() const override;

   /// This is the aim (HEAD) rotation to use.
   /// we don't need to override this because we are going to apply the smoothed head rotation to the ControlRotation.
   ///virtual FRotator GetControlRotation() const override;

   /// Overriding so we can update smoothed rotations
   virtual void UpdateControlRotation(float deltaTime, bool updatePawn = true) override;

   /// Overriding so we don't grab actor location but where we think the actor is
   virtual FVector GetFocalPointOnActor(const AActor* targetActor) const override;

   /// Should be called by a Behavior Tree Task when the currently executing behavior tree
   /// is finished and wants to notify the owning AI State.
   UFUNCTION(BlueprintCallable, Category = "AI|OSE")
   void OnBehaviorTreeFinished();

   UFUNCTION(BlueprintGetter, Category = "AI|OSE")
   AOSECharacterBase* GetOSECharacter() const { return _oseCharacter; }

#if ENABLE_VISUAL_LOG
   virtual void GrabDebugSnapshot(FVisualLogEntry* snapshot) const override;
#endif

   UFUNCTION(BlueprintPure, Category = "AI|OSE")
   UOSEAIPerceptionComponent* GetOSEAIPerceptionComponent() const { return _cachedOSEPerceptionComponent; }

   UFUNCTION(BlueprintPure, Category = "AI|OSE")
   UOSEKnowledgeComponent* GetOSEKnowledgeComponent() const { return _cachedKnowledgeComponent; }

   UFUNCTION(BlueprintPure, Category = "AI|OSE")
   UUtilityAIGoalComponent* GetUtilityAIGoalComponent() const { return _cachedUtilityAIGoalComponent; }

   UFUNCTION(BlueprintPure, Category = "AI|OSE")
   UUtilityAIBehaviorComponent* GetUtilityAIBehaviorComponent() const { return _cachedUtilityAIBehaviorComponent; }


   UPROPERTY(BlueprintAssignable, Category = "AI|OSE")
   FOnPossessedPawn OnPossessedPawn;

   UPROPERTY(BlueprintAssignable, Category = "AI|OSE")
   FOnUnPossessedPawn OnUnPossessedPawn;

   /// from IOSEAlertnessInterface
   virtual UOSEAlertnessComponent* GetAlertnessComponent() const override;
   virtual EAlertnessLevel GetAlertnessLevel() const override;

   void SetForcedControlRotation(const FRotator& rotator);
   void ClearForcedControlRotation();

protected:
   TOptional<FRotator> _forcedRotation;
   
   /// Overriding so we can reset smoothed rotations
   virtual void OnPossess(APawn* inPawn) override;
   virtual void OnUnPossess() override;

   UFUNCTION(BlueprintGetter, Category = "AI|OSE")
   virtual float GetInterpSpeedBody() const { return _interpSpeedBody; }

   UFUNCTION(BlueprintSetter, Category = "AI|OSE")
   virtual void SetInterpSpeedBody(float inValue) { _interpSpeedBody = inValue; }

   UFUNCTION(BlueprintGetter, Category = "AI|OSE")
   virtual float GetInterpSpeedHead() const { return _interpSpeedHead; }

   UFUNCTION(BlueprintSetter, Category = "AI|OSE")
   virtual void SetInterpSpeedHead(float inValue) { _interpSpeedHead = inValue; }

   UFUNCTION(BlueprintGetter, Category = "AI|OSE")
   virtual float GetTargetVelocityUncertainty() const { return _targetVelocityUncertainty; }

   UFUNCTION(BlueprintSetter, Category = "AI|OSE")
   virtual void SetTargetVelocityUncertainty(float inValue) { _targetVelocityUncertainty = inValue; }

   UFUNCTION(BlueprintGetter, Category = "AI|OSE")
   virtual float GetInterceptSpeed() const { return _interceptSpeed; }

   UFUNCTION(BlueprintSetter, Category = "AI|OSE")
   virtual void SetInterceptSpeed(float inValue) { _interceptSpeed = inValue; }

   UFUNCTION(BlueprintGetter, Category = "AI|OSE")
   virtual FVector GetInterceptGravity() const { return _interceptGravity; }

   UFUNCTION(BlueprintSetter, Category = "AI|OSE")
   virtual void SetInterceptGravity(FVector inValue) { _interceptGravity = inValue;  }

private:

   /// The OSECharacterBase currently possessed by this Controller, or nullptr if our Pawn
   /// does not derive from OSECharacterBase.
   UPROPERTY(BlueprintGetter = GetOSECharacter, Category = "AI|OSE")
   AOSECharacterBase* _oseCharacter = nullptr;

   /// Cached behavior component, could be null, assumed it's added in data on controllers that need it
   UPROPERTY(Transient)
   UUtilityAIBehaviorComponent* _cachedUtilityAIBehaviorComponent = nullptr;

   /// Cached goal component, could be null, assumed it's added in data on controllers that need it
   UPROPERTY(Transient)
   UUtilityAIGoalComponent* _cachedUtilityAIGoalComponent = nullptr;

   /// Cached perception component, could be null, assumed it's added in data on controllers that need it
   UPROPERTY(Transient)
   UOSEAIPerceptionComponent* _cachedOSEPerceptionComponent = nullptr;

   /// Cached knowledge component, could be null, assumed it's added in data on controllers that need it
   UPROPERTY(Transient)
   UOSEKnowledgeComponent* _cachedKnowledgeComponent = nullptr;

   /// How unsure an AI is of the velocity of a target, the higher the number, the more known velocity will be distorted.
   /// This is used when determining how to lead the target. If `_interceptSpeed` is 0, this value will not be used either.
   UPROPERTY(EditAnywhere, Category = "AI|OSE", BlueprintGetter = GetTargetVelocityUncertainty, BlueprintSetter = SetTargetVelocityUncertainty, meta = (ClampMin = 0.0, UIMin = 0.0))
   float _targetVelocityUncertainty = 0.0f;

   /// Trace speed for purposes of tracing a focal point from our focus actor.
   /// If > 0, the derived focal point will lead the focal actor's movement.
   UPROPERTY(EditAnywhere, Category = "AI|OSE", BlueprintGetter = GetInterceptSpeed, BlueprintSetter = SetInterceptSpeed, meta = (ClampMin = 0, UIMin = 0))
   float _interceptSpeed;

   /// Trace acceleration for purposes of tracing a focal point from our focus actor.
   /// If nonzero, the derived focal point will account for drop over the course of the trace due to gravity.
   /// Therefore, no effect unless we also have a positive intercept speed.
   UPROPERTY(EditAnywhere, Category = "AI|OSE", BlueprintGetter = GetInterceptGravity, BlueprintSetter = SetInterceptGravity)
   FVector _interceptGravity;

   /// Damping rate for body interpolation. Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag)
   UPROPERTY(EditAnywhere, Category = "AI|OSE", BlueprintGetter = GetInterpSpeedBody, BlueprintSetter = SetInterpSpeedBody, meta = (ClampMin = 0, UIMin = 0))
   float _interpSpeedBody;

   /// Damping rate for head interpolation. Low values are slower (more lag), high values are faster (less lag), while zero is instant (no lag)
   UPROPERTY(EditAnywhere, Category = "AI|OSE", BlueprintGetter = GetInterpSpeedHead, BlueprintSetter = SetInterpSpeedHead, meta = (ClampMin = 0, UIMin = 0))
   float _interpSpeedHead;

   /// Minimum head/control rotation pitch, in degrees
   UPROPERTY(EditAnywhere, Category = "AI|OSE")
   float _headRotationPitchMin = -30.0;

   /// Maximum head/control rotation pitch, in degrees
   UPROPERTY(EditAnywhere, Category = "AI|OSE")
   float _headRotationPitchMax = 70.0f;

   /// Raw desired (HEAD) rotation.  Default value is the base class control rotation
   UPROPERTY(Transient, DuplicateTransient)
   FRotator _rawHeadRotation;

   /// Raw desired (BODY) rotation. Default value is the base class control rotation
   UPROPERTY(Transient, DuplicateTransient)
   FRotator _rawBodyRotation;

   /// Smoothed desired (BODY) rotation.  Accessed via GetDesiredRotation()
   UPROPERTY(Transient, DuplicateTransient)
   FRotator _smoothedBodyRotation;

   /// Smoothed control (HEAD) rotation. Accessed via Super::GetControlRotation()
   UPROPERTY(Transient, DuplicateTransient)
   FRotator _smoothedHeadRotation;
};
