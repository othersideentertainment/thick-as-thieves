// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/Throwable/TATThrowableWorld.h"

// tat
#include "AI/Perception/TATAISense_Hearing.h"
#include "Damage/TATDamageTypes.h"
#include "Items/TATItemFunctionLibrary.h"
#include "Player/TATPlayerStatsTags.h"

// ose
#include "Player/OSEPlayerStats.h"

// ue
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThrowableWorld)

// Sets default values
ATATThrowableWorld::ATATThrowableWorld()
{
    // Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
   PrimaryActorTick.bCanEverTick = true; // TODO: change back once temp interpolation is removed
   PrimaryActorTick.bStartWithTickEnabled = false;

   _viewTargetInterpolationRate_TEMP = 2;
   _dropSpeed_TEMP = 500;

   // these were already set in the BP
   bReplicates = true;
   NetDormancy = DORM_Initial;
}

// Called when the game starts or when spawned
void ATATThrowableWorld::BeginPlay()
{
   Super::BeginPlay();
   
}

void ATATThrowableWorld::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATThrowableWorld, _broken);
}

bool ATATThrowableWorld::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   return UTATItemFunctionLibrary::CanCharacterPickUpThings(interactingCharacter) && !_broken;
}

void ATATThrowableWorld::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   if (AActor* parent = GetAttachParentActor())
   {
      if (_interpolateToViewOffset_TEMP)
      {
         _viewOffset_TEMP = FMath::VInterpTo(_viewOffset_TEMP, _targetViewOffset_TEMP, deltaTime, _viewTargetInterpolationRate_TEMP);
      }

      if (_maintainViewOffset_TEMP)
      {
         FVector viewPosition;
         FRotator viewRotation;
         parent->GetActorEyesViewPoint(viewPosition, viewRotation);
         FVector targetPosition = viewPosition + viewRotation.RotateVector(_viewOffset_TEMP);
         RootComponent->SetWorldLocationAndRotation(targetPosition, viewRotation);
      }
   }

   if (_interpolateToWorldPosition_TEMP)
   {

      FVector newPosition = FMath::VInterpConstantTo(GetActorLocation(), _targetWorldPosition_TEMP, deltaTime, _dropSpeed_TEMP);
      FRotator newRotation = FMath::RInterpTo(GetActorRotation(), _targetWorldRotation_TEMP, deltaTime, 10.f);
      RootComponent->SetWorldLocationAndRotation(newPosition, newRotation);
      if (newPosition == _targetWorldPosition_TEMP)
      {
         _interpolateToWorldPosition_TEMP = false;
         _lastWorldEndTime_TEMP = GetWorld()->GetTimeSeconds() + 0.5f;
         PrimaryActorTick.SetTickFunctionEnable(false);
      }
   }
}

void ATATThrowableWorld::PostNetReceiveLocationAndRotation()
{
   // Obviously this should all get replace once dropping spawns a new actor rather than hanging onto
   // this one for the visualization of the held actor
   if (!_interpolateToWorldPosition_TEMP && !(_lastWorldEndTime_TEMP && _lastWorldEndTime_TEMP > GetWorld()->GetTimeSeconds()))
   {
      Super::PostNetReceiveLocationAndRotation();
   }
}

void ATATThrowableWorld::LocallyMaintainViewOffset_TEMP(FVector viewOffset)
{
   _maintainViewOffset_TEMP = true;
   _viewOffset_TEMP = viewOffset;
   PrimaryActorTick.SetTickFunctionEnable(true);
}

void ATATThrowableWorld::LocallyInterpolateViewOffset_TEMP(FVector viewOffset)
{
   _interpolateToViewOffset_TEMP = true;
   _targetViewOffset_TEMP = viewOffset;
   PrimaryActorTick.SetTickFunctionEnable(true);
}

void ATATThrowableWorld::Drop_TEMP(FVector worldPosition, FRotator worldRotation)
{
   _interpolateToWorldPosition_TEMP = true;
   _maintainViewOffset_TEMP = false;
   _interpolateToViewOffset_TEMP = false;
   _targetWorldPosition_TEMP = worldPosition;
   _targetWorldRotation_TEMP = worldRotation;
   PrimaryActorTick.SetTickFunctionEnable(true);
}

void ATATThrowableWorld::StopMovement_TEMP()
{
   _interpolateToWorldPosition_TEMP = false;
   _maintainViewOffset_TEMP = false;
   _interpolateToViewOffset_TEMP = false;
   PrimaryActorTick.SetTickFunctionEnable(false);
}

bool ATATThrowableWorld::CanHandleDamage_Implementation() const
{
   return _canBreak && !_broken;
}

void ATATThrowableWorld::AuthorityHandleDamage_Implementation(const FTATDamageWithType& damage,
   const FTATSimpleDamageSource& damageSource)
{
   if(damage.DamageAmount >= _minDamageToBreak)
   {
      FlushNetDormancy();
      _broken = true;
      BP_OnBroken();

      UOSEPlayerStatsFunctionLibrary::AuthorityUpdatePlayerStatInt(damageSource.SourceActor.Get(), TAG_PlayerStats_Objects_Broken);

      if(_breakStimTag.IsValid())
      {
         UTATAISense_Hearing::ReportNoiseEvent(this, _breakStimTag, GetActorLocation(), this);
      }

      if(_lifespanAfterBreaking > 0)
      {
         SetLifeSpan(_lifespanAfterBreaking);
      }
      else
      {
         Destroy();
      }
   }
}

void ATATThrowableWorld::_OnRep_Broken()
{
   if(_broken)
   {
      BP_OnBroken();
   }
}


