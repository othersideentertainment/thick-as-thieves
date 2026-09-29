// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Environment/TATShockFloor.h"

// tat
#include "Interactables/Electrical/TATElectricalDeviceComponent.h"

// ose
#include "AI/OSEAIFunctionLibrary.h"
#include "Interactables/OSEInteractionHelpers.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayTagAssetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATShockFloor)

// Sets default values
ATATShockFloor::ATATShockFloor()
{
   bReplicates = true;
   NetDormancy = DORM_Initial;
   
   _chargingDuration = 1.f;
   
   _electricalDeviceComponent = CreateOptionalDefaultSubobject<UTATElectricalDeviceComponent>("ElectricalDevice");
   if (_electricalDeviceComponent)
   {
      _electricalDeviceComponent->SetRequiresPowerSource(false);
   }
}

// Called when the game starts or when spawned
void ATATShockFloor::BeginPlay()
{
	Super::BeginPlay();

   if (HasAuthority())
   {
      if(_electricalDeviceComponent && _electricalDeviceComponent->GetPowerSource())
      {
         _electricalDeviceComponent->OnPoweredChanged.AddUniqueDynamic(this, &ThisClass::_OnPoweredChanged);
         SetOn(_electricalDeviceComponent->IsPowered());
      }
   }
   _SampleSequence(false);
   _UpdateOverlapCollision(HasAuthority() && IsInState(ETATOnOffState::On));
   BP_OnFloorStateChanged(GetState(), GetState(), false);
}

void ATATShockFloor::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   _CancelTimer();
   if (endPlayReason == EEndPlayReason::Destroyed)
   {
      _appliedEffects.CancelAll();
   }
   Super::EndPlay(endPlayReason);
}

void ATATShockFloor::NotifyActorBeginOverlap(AActor* otherActor)
{
   Super::NotifyActorBeginOverlap(otherActor);

   if(!HasAuthority() && !IsInState(ETATOnOffState::On))
   {
      return;
   }

   if(_IsValidTarget(otherActor) && _shockedEffect.Get())
   {
      UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(otherActor, false);
      FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
      effectContext.AddInstigator(this, this);

      // Grant effects to overlapping actor's ASC
      FGameplayEffectSpec effectSpec(_shockedEffect.GetDefaultObject(), effectContext);
      FActiveGameplayEffectHandle activeEffectHandle = asc->ApplyGameplayEffectSpecToSelf(effectSpec);
      if (activeEffectHandle.IsValid())
      {
         _appliedEffects.Add(otherActor, activeEffectHandle);
      }
   }
}

void ATATShockFloor::NotifyActorEndOverlap(AActor* otherActor)
{
   Super::NotifyActorEndOverlap(otherActor);

   _appliedEffects.CancelByActor(otherActor);
}

void ATATShockFloor::_OnStateChanged(bool isOn, bool bWasRecent)
{
   Super::_OnStateChanged(isOn, bWasRecent);

   if(isOn)
   {
      _SampleSequence(bWasRecent);
   }
   else
   {
      _CancelTimer();
      _SetFloorState(ETATOnOffState::Off, bWasRecent);
   }
}

void ATATShockFloor::_CancelTimer()
{
   if(_nextStateTimer.IsValid())
   {
      GetWorldTimerManager().ClearTimer(_nextStateTimer);
   }
}

bool ATATShockFloor::_IsValidTarget(const AActor* actor) const
{
   if(const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(actor))
   {
      return tagInterface->HasAllMatchingGameplayTags(_requiredTargetTags) && !tagInterface->HasAnyMatchingGameplayTags(_blockedTargetTags);
   }

   return false;
}

void ATATShockFloor::_UpdateOverlapCollision(bool enabled)
{
   static const FName kOverlapCollisionTag("FloorOverlap");
   ForEachComponent<UPrimitiveComponent>(false, [enabled](UPrimitiveComponent* primitive)
   {
      if(primitive->ComponentHasTag(kOverlapCollisionTag))
      {
         primitive->SetGenerateOverlapEvents(enabled);
         primitive->SetCollisionEnabled(enabled ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
      }
   });
}

void ATATShockFloor::_OnPoweredChanged(bool isPowered)
{
   SetOn(isPowered);
}

void ATATShockFloor::_SampleSequence(bool wasRecent)
{
   if(!IsOn())
   {
      return;
   }

   const float now = UOSEInteractionHelpers::GetServerTimeForComparison(this);
   TATOnOffSequence::FSampleResult result = TATOnOffSequence::Sample(_GetCompiledSequence(), {
      .StartTime = State.ChangedServerTime,
      .CurrentTime = now
   });

   if(!ensure(result.Success))
   {
      return;
   }

   _SetFloorState(result.State, wasRecent);

   if(result.ShouldScheduleTimer())
   {
      GetWorldTimerManager().SetTimer(_nextStateTimer, FTimerDelegate::CreateUObject(this, &ThisClass::_SampleSequence, true),
         result.GetNextTimerDelay(now), false);
   }
}

void ATATShockFloor::_SetFloorState(ETATOnOffState newState, bool wasRecent)
{
   if(_floorState != newState)
   {
      const ETATOnOffState oldState = _floorState;
      _floorState = newState;
      BP_OnFloorStateChanged(newState, oldState, wasRecent);

      if(HasAuthority())
      {
         _UpdateOverlapCollision(IsInState(ETATOnOffState::On));
      }
   }
}

const TATOnOffSequence::FCompiledSequence& ATATShockFloor::_GetCompiledSequence() const
{
   if(_sequence)
   {
      return _sequence->GetCompiled(_chargingDuration);
   }
   else
   {
      return TATOnOffSequence::GetIndefiniteOn(_chargingDuration);
   }
}

bool ATATShockFloor::CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor, const bool* wasVisible, int32* userData) const
{
   // get a bounding box and trace to the closest point on the outside of the box to check for vis
   static const bool kNonColliding = false;
   static const bool kIncludeFromChildActors = false;
   const FBox actorBounds = GetComponentsBoundingBox(kNonColliding, kIncludeFromChildActors);
   FVector closestPointOnBox = actorBounds.GetClosestPointTo(observerLocation);

   FHitResult hitResult;
   const bool canBeSeen = UOSEAIFunctionLibrary::SightSenseLineTrace(hitResult, observerLocation, this, ignoreActor, &closestPointOnBox);
   numberOfLoSChecksPerformed = 1;
   if (canBeSeen)
   {
      outSeenLocation = GetActorLocation();
      outSightStrength = 1.0f; // TODO: We aren't using this for anything, so...?
      return true;
   }

   outSightStrength = 0;
   return false;
}



