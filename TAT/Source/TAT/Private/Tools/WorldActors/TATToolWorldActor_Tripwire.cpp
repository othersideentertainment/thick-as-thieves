// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/WorldActors/TATToolWorldActor_Tripwire.h"

// tat
#include "AI/Perception/TATAISense_Hearing.h"

// ose
#include "Interactables/OSEInteractionHelpers.h"
#include "Utl/OSEShapeCollisionTrackerComponent.h"
#include "Abilities/OSEAbilitySystemComponent.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Components/CapsuleComponent.h"
#include "Damage/TATDamageFunctionLibrary.h"
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolWorldActor_Tripwire)

ATATToolWorldActor_Tripwire::ATATToolWorldActor_Tripwire()
{
   NetDormancy = DORM_DormantAll;

   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = true;
   PrimaryActorTick.bAllowTickOnDedicatedServer = false;

   _collisionTrackerComponent = CreateDefaultSubobject<UOSEShapeCollisionTrackerComponent>(TEXT("CollisionShapeTracker"));
   _collisionTrackerComponent->OnlyRunOnAuthority = true;
}

void ATATToolWorldActor_Tripwire::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(ATATToolWorldActor_Tripwire, _tripwireLength, COND_InitialOnly);
   DOREPLIFETIME(ATATToolWorldActor_Tripwire, _triggeredTimestamp);
}

#if WITH_EDITOR
EDataValidationResult ATATToolWorldActor_Tripwire::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (!GetClass()->HasAnyClassFlags(CLASS_Abstract) && TriggerTargetCriteria.IsEmpty())
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("TATToolWorldActor_Tripwire '%s' has an empty TriggerTargetCriteria"), *GetName())));
      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif

// static
ATATToolWorldActor_Tripwire* ATATToolWorldActor_Tripwire::AuthoritySpawnAndDeployTripwireActor(AActor* tripwireOwner,
   TSubclassOf<ATATToolWorldActor_Tripwire> tripwireClass, const FVector& tripwireWorldLocation, const FVector& tripwireDirection,
   const FTATGearWorldActorParameters& worldActorParams, bool debugDrawTrace, float debugDrawTraceDuration)
{
   if (!tripwireOwner || !tripwireClass)
   {
      return nullptr;
   }

   check(tripwireOwner->HasAuthority());

   // Build an initial TripwireParams that will be used to compute the spawned actor's transform
   const FTransform transform{ FRotationMatrix::MakeFromX(tripwireDirection.GetSafeNormal()).ToQuat(), tripwireWorldLocation };

   APawn* instigator = Cast<APawn>(tripwireOwner);
   ATATToolWorldActor_Tripwire* tripwire = tripwireOwner->GetWorld()->SpawnActorDeferred<ATATToolWorldActor_Tripwire>(
      tripwireClass,
      transform,
      tripwireOwner,
      instigator,
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn,
      ESpawnActorScaleMethod::OverrideRootScale);

   if (!ensure(tripwire != nullptr))
   {
      return nullptr;
   }

   // Compute the tripwire's location, direction, and desired length
   FTATTripwireParams tripwireParams{
      transform.TransformPosition(tripwire->TripwireRelativeOriginOffset),
      transform.TransformVector(FVector::ForwardVector),
      tripwire->MaxTripwireLength,
   };

   // Clamp the length based on the amount of available space
   FHitResult hitResult{};
   if (TraceTripwirePath(tripwireOwner, hitResult, tripwireParams, tripwire->TripwireTraceProfile, tripwire->TripwireTraceComplexCollision,
      tripwireOwner, tripwire, debugDrawTrace, debugDrawTraceDuration))
   {
      if (hitResult.Distance < tripwireParams.Length)
      {
         tripwireParams.Length = hitResult.Distance;
      }
   }

   // Make sure the actor gets the tripwire length value before we finish spawning so it's included with initial replication
   tripwire->_tripwireLength = tripwireParams.Length;

   ITATGearWorldActorInterface::Execute_AuthorityDeploy(tripwire, worldActorParams);

   tripwire->FinishSpawning(transform);

   return tripwire;
}

void ATATToolWorldActor_Tripwire::BeginPlay()
{
   Super::BeginPlay();

   if (HasAuthority())
   {
      OnSetupTripwire(GetTripwireParams());

      _collisionTrackerComponent->OnActorEnteredShape.AddUniqueDynamic(this, &ATATToolWorldActor_Tripwire::_AuthorityOnActorEnterShape);
   }
}

// static
bool ATATToolWorldActor_Tripwire::TraceTripwirePath(UObject* contextObject, FHitResult& outHitResult, const FTATTripwireParams& tripwireParams,
   FCollisionProfileName collisionProfile, bool traceComplex, AActor* ignoreActor1, AActor* ignoreActor2, bool drawDebug, float debugDrawDuration)
{
   UWorld* world = GEngine->GetWorldFromContextObject(contextObject, EGetWorldErrorMode::ReturnNull);
   if (world == nullptr)
   {
      outHitResult = FHitResult{ ForceInit };
      return false;
   }

   FCollisionQueryParams params{};
   static const FName tripwirePathTraceName = FName(TEXT("TraceTripwirePath"));
   params.TraceTag = tripwirePathTraceName;
   params.bTraceComplex = traceComplex;
   if (ignoreActor1 != nullptr)
   {
      params.AddIgnoredActor(ignoreActor1);
   }
   if (ignoreActor2 != nullptr)
   {
      params.AddIgnoredActor(ignoreActor2);
   }

   const bool hit = world->LineTraceSingleByProfile(outHitResult, tripwireParams.GetStart(), tripwireParams.GetEnd(), collisionProfile.Name, params);

   if (drawDebug)
   {
      const FMatrix circleTransformMatrix = FRotationMatrix::MakeFromX(tripwireParams.Direction) * FTranslationMatrix::Make(tripwireParams.Location);
      DrawDebugCircle(world, circleTransformMatrix, 40.0f, 16, FColor::Purple, false, debugDrawDuration);

      if (hit)
      {
         DrawDebugLine(world, outHitResult.TraceStart, outHitResult.Location, FColor::Green, false, debugDrawDuration);
         DrawDebugPoint(world, outHitResult.Location, 15.0f, FColor::Green, false, debugDrawDuration);
         DrawDebugLine(world, outHitResult.Location, outHitResult.TraceEnd, FColor::Yellow, false, debugDrawDuration);
      }
      else
      {
         DrawDebugLine(world, outHitResult.TraceStart, outHitResult.TraceEnd, FColor::Red, false, debugDrawDuration);
      }
   }

   return hit;
}

void ATATToolWorldActor_Tripwire::GetTripwireWorldLocation(FVector& startLocation, FVector& endLocation) const
{
   const FTATTripwireParams params = GetTripwireParams();
   startLocation = params.GetStart();
   endLocation = params.GetEnd();
}

FTATTripwireParams ATATToolWorldActor_Tripwire::GetTripwireParams() const
{
   const FTransform actorTransform = GetActorTransform();
   return FTATTripwireParams{
      actorTransform.TransformPosition(TripwireRelativeOriginOffset),
      actorTransform.TransformVector(FVector::ForwardVector),
      FMath::Clamp(_tripwireLength, MinTripwireLength, MaxTripwireLength),
   };
}

namespace TripwireHelpers
{
   UPrimitiveComponent* GetHitActorComponent(AActor* actor)
   {
      if (actor == nullptr)
      {
         return nullptr;
      }
      if (UPrimitiveComponent* rootComp = Cast<UPrimitiveComponent>(actor->GetRootComponent()))
      {
         return rootComp;
      }
      if (ACharacter* character = Cast<ACharacter>(actor))
      {
         return character->GetCapsuleComponent();
      }
      return actor->GetComponentByClass<UPrimitiveComponent>();
   }

   void TripwireParamsToHitResult(FHitResult& outHitResult, const FTATTripwireParams& tripwireParams, AActor* hitActor, UPrimitiveComponent* hitComponent)
   {
      if (hitActor == nullptr || hitComponent == nullptr)
      {
         outHitResult = FHitResult{ tripwireParams.GetStart(), tripwireParams.GetEnd() };
         return;
      }
      const FVector hitNormal = (hitActor->GetActorLocation() - tripwireParams.GetStart()).GetSafeNormal();
      outHitResult = FHitResult{ hitActor, hitComponent, hitActor->GetActorLocation(), hitNormal };
      outHitResult.bBlockingHit = true;
      outHitResult.TraceStart = tripwireParams.GetStart();
      outHitResult.TraceEnd = tripwireParams.GetEnd();
      outHitResult.Distance = FVector::Distance(outHitResult.TraceStart, outHitResult.Location);
      check(outHitResult.TraceStart != outHitResult.TraceEnd);
      outHitResult.Time = outHitResult.Distance / FVector::Distance(outHitResult.TraceStart, outHitResult.TraceEnd);
   }
}

void ATATToolWorldActor_Tripwire::_OnAuthorityTriggeredBy(AActor* actor, UAbilitySystemComponent* asc)
{
   check(actor != nullptr);

   if (TripwireTriggerHearingStim.IsValid())
   {
      UTATAISense_Hearing::ReportNoiseEvent(this, TripwireTriggerHearingStim, actor->GetActorLocation(), this);
   }

   if (TripwireDamage.DamageAmount > 0 && TripwireDamage.DamageType.IsValid())
   {
      UPrimitiveComponent* actorComp = TripwireHelpers::GetHitActorComponent(actor);
      if (actorComp != nullptr && !UTATDamageFunctionLibrary::IsComponentInvalidDamageTarget(actorComp))
      {
         FHitResult hitResult;
         TripwireHelpers::TripwireParamsToHitResult(hitResult, GetTripwireParams(), actor, actorComp);
         UTATDamageFunctionLibrary::DealDamage(this, actor, TripwireDamage, GetActorLocation(), hitResult);
      }
   }

   if (TripwireEffect && asc != nullptr)
   {
      asc->ApplyGameplayEffectToSelf(TripwireEffect.GetDefaultObject(), 0.0f, asc->MakeEffectContext());
   }

   OnAuthorityTriggeredBy(actor);
}

void ATATToolWorldActor_Tripwire::_OnRep_TripwireLength()
{
   if (!HasAuthority())
   {
      OnSetupTripwire(GetTripwireParams());
   }
}

void ATATToolWorldActor_Tripwire::_AuthorityOnActorEnterShape(AActor* actor)
{
   check(HasAuthority());

   // If we've already been triggered, ignore further cases
   if (_triggeredTimestamp >= 0.0f)
   {
      return;
   }

   if (!ensure(actor != nullptr))
   {
      return;
   }

   // Ignore our instigator
   if (TriggerIgnoreInstigator && actor == GetInstigator())
   {
      return;
   }

   // Check if the target has the necessary tags
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor))
   {
      FGameplayTagContainer tagContainer;
      asc->GetOwnedGameplayTags(tagContainer);

      if (TriggerTargetCriteria.Matches(tagContainer))
      {
         _OnAuthorityTriggeredBy(actor, asc);

         static constexpr bool justHappened = true;
         OnTriggered(justHappened);

         FlushNetDormancy();
         _triggeredTimestamp = UOSEInteractionHelpers::GetServerTimeForWrite(GetWorld());

         // Stop tracking overlaps since we've triggered
         _collisionTrackerComponent->OnActorEnteredShape.RemoveAll(this);
         _collisionTrackerComponent->ClearShapeTracking();
      }
   }
}

void ATATToolWorldActor_Tripwire::_OnRep_TriggeredTimestamp()
{
   if (_triggeredTimestamp >= 0.0f)
   {
      // It's possible due to net relevancy that we don't hear about the change for some time
      // In that case, we'd still want any state changes to occur, but transient VFX would know to not play
      bool justHappened = !UOSEInteractionHelpers::IsOld(GetWorld(), _triggeredTimestamp);
      OnTriggered(justHappened);
   }
}
