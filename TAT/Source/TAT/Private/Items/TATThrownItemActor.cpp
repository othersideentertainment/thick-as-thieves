// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/TATThrownItemActor.h"

// ue4
#include "GameFramework/ProjectileMovementComponent.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThrownItemActor)

ATATThrownItemActor::ATATThrownItemActor()
{
   _projectileMovement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovement"));
   _projectileMovement->bInterpMovement = true;
   _projectileMovement->bInterpRotation = true;

   // Don't use the projectile movement unless actually thrown
   _projectileMovement->bAutoRegisterUpdatedComponent = false;

   // While the client simulating as well as interpolating looks better,
   // it can diverge
   _projectileMovement->bSimulationEnabled = false;
}

void ATATThrownItemActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATThrownItemActor, _inFlight);
}

void ATATThrownItemActor::AuthorityStartThrowing(const FVector& velocity)
{
   check(HasAuthority())
   SetReplicateMovement(true);
   _AuthoritySetInFlight(true);
   _projectileMovement->OnProjectileStop.AddUniqueDynamic(this, &ATATThrownItemActor::_OnProjectileStop);
   _projectileMovement->bSimulationEnabled = true;
   _projectileMovement->SetUpdatedComponent(GetRootComponent());
   _projectileMovement->Velocity = velocity;
   BP_OnAuthorityThrowStart();

   if (_rotateDuringThrow)
   {
      _targetRotation = FRotator(GetRootComponent()->GetRelativeRotation().Quaternion() * _desiredRelativeRotation.Quaternion());
      SetActorTickEnabled(true);
   }
}

void ATATThrownItemActor::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   // This simple interpolation of the rotation looks okay except of ramps
   //
   if (_projectileMovement && _projectileMovement->UpdatedComponent && HasAuthority())
   {
      USceneComponent* root = GetRootComponent();
      const FRotator newRotation = FMath::RInterpConstantTo(root->GetRelativeRotation(), _targetRotation, deltaTime, _throwRotationRate);
      root->SetRelativeRotation(newRotation, true);
      if (newRotation == _targetRotation && !_IsBeingTaken())
      {
         SetActorTickEnabled(false);
      }
   }
}

void ATATThrownItemActor::BeginPlay()
{
   Super::BeginPlay();

   // Ignore collision with instigator and vice versa
   if (AActor* instigator = GetInstigator())
   {
      if (auto myPrimitive = Cast<UPrimitiveComponent>(RootComponent))
      {
         myPrimitive->IgnoreActorWhenMoving(instigator, true);
      }
      if (auto theirPrimitive = Cast<UPrimitiveComponent>(instigator->GetRootComponent()))
      {
         theirPrimitive->IgnoreActorWhenMoving(this, true);
      }
   }
}

void ATATThrownItemActor::PostNetReceiveLocationAndRotation()
{
   // Do not clobber position if being taken so as not to cause visual stuttering
   if (_IsBeingTaken())
   {
      return;
   }

   if (_projectileMovement && _projectileMovement->UpdatedComponent && _projectileMovement->bInterpMovement)
   {
      const FRepMovement& constRepMovement = GetReplicatedMovement();
      const FVector newLocation = FRepMovement::RebaseOntoLocalOrigin(constRepMovement.Location, this);
      _projectileMovement->MoveInterpolationTarget(newLocation, constRepMovement.Rotation);
   }
   else
   {
      Super::PostNetReceiveLocationAndRotation();
   }
}

void ATATThrownItemActor::PostNetReceiveVelocity(const FVector& newVelocity)
{
   if (_projectileMovement && _projectileMovement->UpdatedComponent)
   {
      _projectileMovement->Velocity = newVelocity;
   }
   else
   {
      Super::PostNetReceiveVelocity(newVelocity);
   }
}

void ATATThrownItemActor::OnRep_ReplicateMovement()
{
   Super::OnRep_ReplicateMovement();

   // Use replicating movement here rather than a secondary flag so that movement updates do not get lost
   if (IsReplicatingMovement())
   {
      _projectileMovement->SetUpdatedComponent(GetRootComponent());
   }
   else
   {
      _projectileMovement->SetUpdatedComponent(nullptr);
   }
}

void ATATThrownItemActor::_OnTakenAuthority(ACharacter* inTakingCharacter)
{
   Super::_OnTakenAuthority(inTakingCharacter);

   SetReplicateMovement(false);
   if (_inFlight)
   {
      _AuthoritySetInFlight(false);
      _projectileMovement->SetUpdatedComponent(nullptr);
   }
}

void ATATThrownItemActor::_OnProjectileStop(const FHitResult& impactResult)
{
   _AuthoritySetInFlight(false);

   // TODO: can we get away with turning off movement replication or dormancy here?
}

void ATATThrownItemActor::OnRep_InFlight(bool oldValue)
{
   if (_inFlight != oldValue)
   {
      _OnInFlightChanged();
   }
}

void ATATThrownItemActor::_OnInFlightChanged()
{
   BP_OnInFlightChanged(_inFlight);
}

void ATATThrownItemActor::_AuthoritySetInFlight(bool newInFlight)
{
   _inFlight = newInFlight;
   _OnInFlightChanged();
}

