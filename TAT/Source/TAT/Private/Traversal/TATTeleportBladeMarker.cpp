// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traversal/TATTeleportBladeMarker.h"

// tat
#include "Traversal/TATTeleportUtilities.h"

// ue4
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTeleportBladeMarker)

namespace
{
   static FTATTeleportTargetParams MakeTeleportParamsForMarker(const ATATTeleportBladeMarker* marker)
   {
      check(marker);
      return { marker->GetActorLocation(), marker->GetActorForwardVector() };
   }
}

// Sets default values
ATATTeleportBladeMarker::ATATTeleportBladeMarker()
{
   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;

   bReplicates = true;
   NetDormancy = DORM_DormantAll;
   SetNetUpdateFrequency(1);

   _despawnDistance = 3000;
}

// Called when the game starts or when spawned
void ATATTeleportBladeMarker::BeginPlay()
{
   Super::BeginPlay();
   
   // This shouldn't meaningfully race, given that this is only created as a
   // result of player action, and if they are the local player, it should already be present
   const APawn* instigator = GetInstigator();
   if (HasAuthority() || (instigator && instigator->IsLocallyControlled()))
   {
      // tick locally for debug (and maybe non-debug) visualizations
      SetActorTickEnabled(true);
   }
}

// Called every frame
void ATATTeleportBladeMarker::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

   const ACharacter* instigator = GetInstigator<ACharacter>();
   if (instigator && instigator->IsLocallyControlled())
   {
      UTATTeleportUtilities::VisualizeTeleportLocation(instigator, MakeTeleportParamsForMarker(this), _teleportSettings);
   }

   if (HasAuthority() && instigator)
   {
      _CheckDespawnDistance(instigator);
   }
}

bool ATATTeleportBladeMarker::FindTeleportLocation(FVector& outLocation) const
{
   const ACharacter* instigator = GetInstigator<ACharacter>();
   return UTATTeleportUtilities::CalculateTeleportLocation(instigator, MakeTeleportParamsForMarker(this), _teleportSettings, outLocation);
}

bool ATATTeleportBladeMarker::HasTeleportLocationForMarker(const AActor* instigator, TSubclassOf<ATATTeleportBladeMarker> markerClass, const FTransform& markerTransform)
{
   if (markerClass == nullptr)
   {
      return false;
   }

   const ACharacter* instigatorCharacter = Cast<ACharacter>(instigator);
   if (instigatorCharacter == nullptr)
   {
      return false;
   }

   FVector unusedOutLocation;
   const FTATTeleportTargetParams params { markerTransform.GetLocation(), markerTransform.GetUnitAxis(EAxis::X) };
   const FTATTeleportQuerySettings& settings = markerClass.GetDefaultObject()->_teleportSettings;
   return UTATTeleportUtilities::CalculateTeleportLocation(instigatorCharacter, params, settings, unusedOutLocation);
}

void ATATTeleportBladeMarker::_CheckDespawnDistance(const AActor* instigator)
{
   check(instigator);

   if (FVector::DistSquared(GetActorLocation(), instigator->GetActorLocation()) > FMath::Square(_despawnDistance))
   {
      _OnExceededDespawnDistance(instigator);
   }
}

void ATATTeleportBladeMarker::_OnExceededDespawnDistance(const AActor* instigator)
{
   check(instigator);
   
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::Get().GetAbilitySystemComponentFromActor(instigator))
   {
      FGameplayCueParameters params;
      params.Location = GetActorLocation();
      params.bReplicateLocationWhenUsingMinimalRepProxy = true;
      asc->ExecuteGameplayCue(_exceededDespawnDistanceTag, params);
   }

   Destroy();
}


