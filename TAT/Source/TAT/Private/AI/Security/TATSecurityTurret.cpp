// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Security/TATSecurityTurret.h"

// tat
#include "AI/Squad/TATSquadAlarmStation.h"
#include "Breakables/TATBreakableComponent.h"
#include "Interactables/TATInteractHighlightUtils.h"
#include "Items/Throwable/TATProjectile.h"
#include "AI/SmartObjects/TATActionNodeComponent_IncorrectObjectState.h"

// ose
#include "Abilities/OSEGameplayAbilitySet.h"
#include "Projectiles/OSEProjectileFunctionLibrary.h"

// ue
#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Perception/AIPerceptionStimuliSourceComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSecurityTurret)

DEFINE_LOG_CATEGORY(LogTATSecurityTurret);


ATATSecurityTurret::ATATSecurityTurret() : Super()
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;
   bReplicates = true;
   _detectSuspiciousActorBehavior = ETATVisionPerceptionDeviceDetectSuspiciousActorBehavior::TrackFirstDetected;
   
   _rootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootSceneComponent"));
   _rootComponent->bEditableWhenInherited = true;
   _rootComponent->Mobility = EComponentMobility::Static;
   RootComponent = _rootComponent;
   
   _incorrectStateActionNodeComponent = CreateDefaultSubobject<UTATActionNodeComponent_IncorrectObjectState>(TEXT("IncorrectStateActionNode"));
   _incorrectStateActionNodeComponent->SetupAttachment(GetRootComponent());
   _incorrectStateActionNodeComponent->Mobility = EComponentMobility::Static;
   
   _perceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSourceComponent"));
}

// Called when the game starts or when spawned
void ATATSecurityTurret::BeginPlay()
{
   Super::BeginPlay();

   _abilitySystemComponent->SetAvatarActor(this);

   // Grant initial abilities
   for (const UOSEGameplayAbilitySet* abilitySet : InitialAbilitySets)
   {
      abilitySet->GiveAbilities(this);
   }
   
   if(const UProjectileMovementComponent* projectileMovementComponent = UOSEProjectileFunctionLibrary::GetProjectileMovementByClass(_projectileType))
   {
      _projectileMaxSpeed = projectileMovementComponent->GetMaxSpeed();
   }
   else
   {
      UE_LOG(LogTATSecurityTurret, Error, TEXT("Define a projectile to use with turret [%s]"), *this->GetName());
   }
   
   //NOTE: Disabled by leadership request - change to State.bIsOn if we want it enabled again.
   _incorrectStateActionNodeComponent->IsEnabled = false;
   _incorrectStateActionNodeComponent->SetInitialState(State.bIsOn, State.bIsOn);
}

void ATATSecurityTurret::PostInitializeComponents()
{
   Super::PostInitializeComponents();
   if (HasAuthority() && _incorrectStateActionNodeComponent != nullptr)
   {
      // Incorrect state component will pay attention to whether this actor is broken or not.
      // If broken, the object will be marked as not in an incorrect state as it's a state 
      // the AI can't do anything about
      _incorrectStateActionNodeComponent->AssignBreakableComponent(_breakableComponent);

      // Incorrect state component will enable/disable the stimuli source component based on
      // whether the object is in a correct/incorrect state. If in incorrect state, it will be 
      // visible. If in a correct state, it will not be visible.
      _incorrectStateActionNodeComponent->AssignStimuliSourceComponent(_perceptionStimuliSource);
   }
}

void ATATSecurityTurret::Tick(const float deltaTime)
{
   Super::Tick(deltaTime);
   
   if(HasAuthority() == false)
      return;
   
   if(IsOn() == false)
      return;
   
   if(_IsDetectingEnemyActor() && _deviceState == ETATVisionPerceptionDeviceState::Triggered)
   {
      const float time = GetWorld()->GetTimeSeconds();
      if(_nextTimeAvailableToFire < time)
      {
         _nextTimeAvailableToFire = time + _fireRate;
         FGameplayEventData eventData;
         eventData.Instigator = this;
         eventData.EventTag = _turretFireGameplayTag;
         UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(this, _turretFireGameplayTag, eventData);
      }
   }
}

AActor* ATATSecurityTurret::GetTargetForFiring() const
{
   ensure(_IsDetectingEnemyActor());
   return _oscillationState.TrackedActor;
}

bool ATATSecurityTurret::IsAllowedToSeeActor(const AActor* actor) const
{
   const bool bCanSee = Super::IsAllowedToSeeActor(actor);
   if(!bCanSee)
      return false;

   // if the projectile isn't in range, the turret can't see them
   FVector velocity;
   if(GetProjectileVelocityForTarget(actor, velocity))
   {
      return bCanSee;      
   }
   
   return false;
}

bool ATATSecurityTurret::GetProjectileVelocityForTarget(const AActor* target, FVector& outVelocity) const
{
   check(target);
   FVector targetPosition;
   FRotator targetRotation;
   target->GetActorEyesViewPoint(targetPosition, targetRotation);

   FVector targetVelocity = target->GetVelocity();
   const float worldTime = GetWorld()->GetTimeSeconds();
   const float targetVelocityUncertainty = _targetDistanceUncertainty * FMath::PerlinNoise1D(worldTime);
   targetVelocity = targetVelocity + (targetVelocity * targetVelocityUncertainty);
   targetPosition = targetPosition + targetVelocity * _targetLeadingAmount;
   const USceneComponent* firingPosition = GetFiringPosition();
   if(firingPosition == nullptr)
      return false;
   if (UGameplayStatics::SuggestProjectileVelocity_CustomArc(this, outVelocity,
                                                             firingPosition->GetComponentLocation(),
                                                             targetPosition, 0.f, 0.9f))
   {
      if(outVelocity.Length() < _projectileMaxSpeed)
      {
         return true;
      }
   }
   return false;
}

bool ATATSecurityTurret::AuthorityIsObjectInCorrectState_Implementation(bool allowIgnoringOfState) const
{
   check(_incorrectStateActionNodeComponent != nullptr);
   return _incorrectStateActionNodeComponent->IsStateCorrect(allowIgnoringOfState);
}

FGameplayTagCountContainer& ATATSecurityTurret::GetGameplayTagCountContainer()
{
   check(_incorrectStateActionNodeComponent != nullptr);
   return _incorrectStateActionNodeComponent->GetIncorrectStateTags();
}

bool ATATSecurityTurret::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   if(_breakableComponent->IsBroken())
      return false;
   if(_allowManuallyChangingPowerState == false)
      return false;
   return true;
}

void ATATSecurityTurret::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   prompt.PressAction = State.bIsOn ? TurnOffPrompt : TurnOnPrompt;
}

FInteractStartResult ATATSecurityTurret::StartInteract_Implementation(ACharacter* InteractingCharacter)
{
   SetOn(!State.bIsOn);
   FInteractStartResult result;
   result.InstantAnimationTag = IsOn() ? TurnOnAnimationTag : TurnOffAnimationTag;
   return result;
}

void ATATSecurityTurret::ShowHighlight_Implementation(bool bShowHighlight)
{
   UTATInteractHighlightUtils::HighlightInteractMeshes(this, bShowHighlight);
}

void ATATSecurityTurret::_OnStateChanged(bool bIsOn, bool bWasRecent)
{
   Super::_OnStateChanged(bIsOn, bWasRecent);
   if (HasAuthority())
   {
      //NOTE: Disabled by leadership request
      //_incorrectStateActionNodeComponent->SetCurrentState(bIsOn ? 1 : 0);
   }
}

void ATATSecurityTurret::SetSettingsForDifficulty(const ETATDifficulty difficulty)
{
   Super::SetSettingsForDifficulty(difficulty);
   if (const FTATSecurityTurretDifficultySettings* settings = _difficultyToTuningMap.Find(difficulty))
   {
      if (settings->UseMultiplierInsteadOfSeconds)
      {
         _fireRate = _fireRate * settings->FireRateMultiplier;
      }
      else
      {
         _fireRate = settings->FireRate;
      }
   }
}

bool ATATSecurityTurret::_HasActiveAlarm() const
{
   for (const ATATSquadAlarmStation* alarm : AlarmStations)
   {
      if (alarm && alarm->GetState() == EAlarmState::Triggered)
      {
         return true;
      }
   }

   return false;
}

bool ATATSecurityTurret::_ShouldTurnOnWhenRepaired() const
{
   return _HasActiveAlarm();
}

void ATATSecurityTurret::_AuthorityOnAlarmSystemStateChanged(EAlarmState newState, EAlarmState previousState,
                                                             bool isRecent)
{
   if (!_breakableComponent->IsBroken())
   {
      SetOn(newState == EAlarmState::Triggered);
   }
   Super::_AuthorityOnAlarmSystemStateChanged(newState, previousState, isRecent);
}
