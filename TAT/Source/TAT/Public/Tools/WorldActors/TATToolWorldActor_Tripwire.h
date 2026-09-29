// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Tools/WorldActors/TATToolWorldActor_Base.h"

// ue5
#include "GameplayTagContainer.h"
#include "Damage/TATDamageTypes.h"
#include "GameFramework/Actor.h"

#include "TATToolWorldActor_Tripwire.generated.h"

class UAbilitySystemComponent;
class UOSEShapeCollisionTrackerComponent;

/// Location and direction of a tripwire
USTRUCT(BlueprintType)
struct FTATTripwireParams
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tripwire Params")
   FVector Location = FVector::ZeroVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tripwire Params")
   FVector Direction = FVector::ForwardVector;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tripwire Params")
   float Length = 1000.0f;

   FTATTripwireParams() = default;

   FTATTripwireParams(const FVector& location, const FVector& direction, float length)
      : Location(location)
      , Direction(direction)
      , Length(length)
   {
   }

   FORCEINLINE FVector GetStart() const { return Location; }
   FORCEINLINE FVector GetEnd() const { return Location + (Direction * Length); }
};

USTRUCT(BlueprintType)
struct TAT_API FTATTripwireState
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tripwire State")
   float WireLength = 0.0f;
};

/// An actor spawned by tools, that applies an effect to another actor when they get too close
UCLASS()
class TAT_API ATATToolWorldActor_Tripwire : public ATATToolWorldActor_Base
{
   GENERATED_BODY()

public:
   ATATToolWorldActor_Tripwire();

   // From UObject
   virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& outLifetimeProps) const override;

#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   /// Spawns a tripwire actor, then calls ITATGearWorldActorInterface::Execute_AuthorityDeploy on the newly spawned actor.
   /// Does a line trace to determine the actual tripwire length, using the length from tripwireParams as a maximum length.
   /// Note that the length is also clamped by the tripwire class's min and max length properties (if enabled).
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "Tool World Actor - Tripwire")
   static ATATToolWorldActor_Tripwire* AuthoritySpawnAndDeployTripwireActor(AActor* tripwireOwner, TSubclassOf<ATATToolWorldActor_Tripwire> tripwireClass,
      const FVector& tripwireWorldLocation, const FVector& tripwireDirection, const FTATGearWorldActorParameters& worldActorParams,
      bool debugDrawTrace = false, float debugDrawTraceDuration = 0.0f);

   // From AActor
   virtual void BeginPlay() override;

   UFUNCTION(BlueprintImplementableEvent)
   void OnSetupTripwire(const FTATTripwireParams& params);

   UFUNCTION(BlueprintCallable, Category = "Tool World Actor - Tripwire", Meta = (WorldContext = "contextObject"))
   static bool TraceTripwirePath(UObject* contextObject, FHitResult& outHitResult, const FTATTripwireParams& tripwireParams, FCollisionProfileName collisionProfile,
      bool traceComplex, AActor* ignoreActor1 = nullptr, AActor* ignoreActor2 = nullptr, bool drawDebug = false, float debugDrawDuration = 0.0f);

   /// Gets the tripwire's start and endpoints in world space.
   UFUNCTION(BlueprintPure, Category = "Tool World Actor - Tripwire")
   void GetTripwireWorldLocation(FVector& startLocation, FVector& endLocation) const;

   UFUNCTION(BlueprintPure, Category = "Tool World Actor - Tripwire")
   FTATTripwireParams GetTripwireParams() const;

   UFUNCTION(BlueprintImplementableEvent)
   void OnAuthorityTriggeredBy(AActor* actor);

   /// Fires on the client and server
   /// The justHappened flag tells us if this is an event that just triggered (and thus should play transient VFX),
   /// or if it happened a while ago and we're just getting the replication
   UFUNCTION(BlueprintImplementableEvent)
   void OnTriggered(bool justHappened);

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Tool World Actor - Tripwire")
   FCollisionProfileName TripwireTraceProfile;

   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Tool World Actor - Tripwire")
   bool TripwireTraceComplexCollision = true;

   /// An actor-relative offset for the tripwire's starting point.
   /// Primarily useful to offset the tripwire a bit to account for art assets.
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Tool World Actor - Tripwire")
   FVector TripwireRelativeOriginOffset = FVector::ZeroVector;

   /// Minimum length of the tripwire.
   /// Note that this value will not account for collision, so keep this small to avoid having the tripwire clipping through walls.
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Tool World Actor - Tripwire")
   float MinTripwireLength = 100.0f;

   /// Maximum length of the tripwire.
   /// This value _does_ account for collision - this is the actual length of the tripwire when there is nothing in the way.
   UPROPERTY(BlueprintReadOnly, EditDefaultsOnly, Category = "Tool World Actor - Tripwire")
   float MaxTripwireLength = 1000.0f;

   /// Damage to apply to an actor triggering the tripwire
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tripwire Target Effect")
   FTATDamageWithType TripwireDamage;

   /// Gameplay effect to apply to an actor triggering the tripwire
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tripwire Target Effect")
   TSubclassOf<UGameplayEffect> TripwireEffect;

   /// Stim to trigger when the tripwire is triggered
   UPROPERTY(EditDefaultsOnly, Category = Stims, meta = (Categories = "AI.Stim.Hearing"))
   FGameplayTag TripwireTriggerHearingStim;

   /// If enabled, prevents the character that placed this tripwire from triggering it
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tripwire Target Effect")
   bool TriggerIgnoreInstigator = true;

   /// Extra target criteria to check before triggering the tripwire
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Tripwire Target Effect")
   FGameplayTagQuery TriggerTargetCriteria;

protected:
   UPROPERTY(Replicated, ReplicatedUsing = _OnRep_TripwireLength, BlueprintReadWrite)
   float _tripwireLength = 0.0f;

   // From ATATToolWorldActor_Base
   virtual bool _HasBeenActivated() const override { return _triggeredTimestamp >= 0.0f; }

   UPROPERTY(EditDefaultsOnly)
   UOSEShapeCollisionTrackerComponent* _collisionTrackerComponent = nullptr;

   void _OnAuthorityTriggeredBy(AActor* actor, UAbilitySystemComponent* asc);

private:
   UFUNCTION()
   void _OnRep_TripwireLength();

   UFUNCTION()
   void _AuthorityOnActorEnterShape(AActor* actor);

   UFUNCTION()
   void _OnRep_TriggeredTimestamp();

   UPROPERTY(Transient, ReplicatedUsing=_OnRep_TriggeredTimestamp)
   float _triggeredTimestamp = -1.0f;
};
