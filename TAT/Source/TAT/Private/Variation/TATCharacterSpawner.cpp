// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/TATCharacterSpawner.h"

// tat
#include "AI/LivingWorld/TATLivingWorldAgentComponent.h"
#include "Character/TATCharacterAIBase.h"
#include "Environment/TATPrivateSpaceCharacterComponent.h"
#include "Environment/TATPrivateSpaceCharacterInterface.h"
#include "Variation/TATSpawnData.h"
#include "Variation/Clues/TATNPCClueSpawnerComponent.h"

// ose
#include "Utl/OSEUtlFunctionLibrary.h"

// ue4
#include "Components/BillboardComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/CollisionProfile.h"
#include "Variation/TATSpawnerComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterSpawner)

ATATCharacterSpawner::ATATCharacterSpawner()
   : Super()
{
   // setup for capsule + arrows mostly copied from PlayerStart + NavigationObjectBase
   
   _capsuleComponent = CreateDefaultSubobject<UCapsuleComponent>(TEXT("CollisionCapsule"));
   _capsuleComponent->ShapeColor = FColor(255, 138, 5, 255);
   _capsuleComponent->bDrawOnlyIfSelected = true;
   _capsuleComponent->InitCapsuleSize(40.0f, 92.0f);
   _capsuleComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
   _capsuleComponent->bShouldCollideWhenPlacing = true;
   _capsuleComponent->SetShouldUpdatePhysicsVolume(true);
   _capsuleComponent->SetMobility(EComponentMobility::Static);
   _capsuleComponent->SetupAttachment(RootComponent);

   _clueSpawnerComponent = CreateDefaultSubobject<UTATNPCClueSpawnerComponent>(TEXT("NPCClueSpawner"));
   _clueSpawnerComponent->bAutoActivate = false; // Only activate if this character spawner actually spawns a character.

#if WITH_EDITORONLY_DATA
   // Add disabled as allowed type for now to characters specifically
   // since it seems like there are valid use-cases to want to disable
   // such a spawner without deleting it.
   // TODO: Is the base code useful at all, or should it just be removed?
   // (also this could totally be a bitmask if we cared about perf at all)
   _validSpawnChanceTypes.Add(ETATSpawnChanceType::Disabled);
#endif
}

void ATATCharacterSpawner::BeginPlay()
{
   Super::BeginPlay();
}

void TryApplyModifiers(const TArray<UTATSpawnModifier*>& modifiers, AActor& actor,  const FRandomStream& randomStream, bool isSpawnFinished)
{
   for (const UTATSpawnModifier* modifier : modifiers)
   {
      if(modifier == nullptr || modifier->ApplyBeforeActorFinishSpawn() == isSpawnFinished)
      {
         continue;
      }
      modifier->ApplyModifier(actor, randomStream);
   }
}

void ATATCharacterSpawner::AuthoritySpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   if (_clueSpawnerComponent)
   {
      // Ensure the clue spawner is activated if this spawner is creating an AI character.
      // That way, the character can be considered for clue-giving.
      _clueSpawnerComponent->Activate();
   }

   Super::AuthoritySpawn(spawnContext, randomStream);
}

AActor* ATATCharacterSpawner::AuthoritySpawnActorDeferred(const FTATVariationSpawnContext& spawnContext, TSubclassOf<AActor> spawnClass, const FRandomStream& randomStream)
{
   if (_clueSpawnerComponent)
   {
      // Ensure the clue spawner is activated if this spawner is creating an AI character.
      // That way, the character can be considered for clue-giving.
      _clueSpawnerComponent->Activate();
   }

   return Super::AuthoritySpawnActorDeferred(spawnContext, spawnClass, randomStream);
}

void ATATCharacterSpawner::AuthorityFinishSpawnActor(const FTATVariationSpawnContext& spawnContext, TSubclassOf<AActor> spawnClass, const FRandomStream& randomStream, AActor* spawnedActorInstance)
{
   Super::AuthorityFinishSpawnActor(spawnContext, spawnClass, randomStream, spawnedActorInstance);
   TryApplyModifiers(_modifiers, *spawnedActorInstance, randomStream, true);

   if (_livingWorldSmartObjectOverrides.Num())
   {
      if (UTATLivingWorldAgentComponent* livingWorldComponent = UTATLivingWorldAgentComponent::Get(spawnedActorInstance))
      {
         UOSEUtlFunctionLibrary::ShuffleWithRandomStream(_livingWorldSmartObjectOverrides, randomStream);
         livingWorldComponent->ForceSmartObjects(_livingWorldSmartObjectOverrides);
      }
   }

   if (ATATCharacterAIBase* character = Cast<ATATCharacterAIBase>(spawnedActorInstance))
   {
      if(ITATPrivateSpaceCharacterInterface* privateSpaceCharacterInterface = Cast<ITATPrivateSpaceCharacterInterface>(character))
      {
         if(UTATPrivateSpaceCharacterComponent* privateSpaceCharacterComponent = privateSpaceCharacterInterface->GetPrivateSpaceCharacterComponent())
         {
            for (const FGameplayTag& allowedPrivateSpaceGameplayTag : _allowedPrivateSpaceGameplayTags)
            {
               privateSpaceCharacterComponent->AuthorityAddAllowedPrivateZone(allowedPrivateSpaceGameplayTag);
            }
         }
      }

      if (_clueSpawnerComponent)
      {
         auto CreateAndInitializeClueComponent = [weakCharacter = MakeWeakObjectPtr(character), clueSpawnerComponent = _clueSpawnerComponent]()
         {
            // Should be valid - it is whats triggering this lambda callback.
            check(clueSpawnerComponent);

            if (ATATCharacterAIBase* character = weakCharacter.Get())
            {
               character->CreateNPCClueComponent(clueSpawnerComponent);
            }

            clueSpawnerComponent->OnSpawnerClueCachedNative.Unbind();
         };

         if (_clueSpawnerComponent->AuthorityHasClueToShare())
         {
            // If a clue has been generated by the time this AI actor has finished spawning,
            // give them a clue interaction component.
            CreateAndInitializeClueComponent();
         }
         else
         {
            // If a clue has not yet been generated by the time this AI actor has finished spawning,
            // wait to see if one is ever generated.
            _clueSpawnerComponent->OnSpawnerClueCachedNative.BindLambda(CreateAndInitializeClueComponent);
         }
      }
      
      // let blueprints set anything up it wants to BEFORE we give them a controller/brain.  this allows any logic like
      // patrol paths that may be setup in OnPossessed to be correctly configured
      AuthorityOnCharacterSpawnFinished(character, randomStream);
      character->SpawnDefaultController();
   }
}

void ATATCharacterSpawner::AuthorityNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   if (_clueSpawnerComponent)
   {
      // Deactivate the clue spawner if this spawner did not create an AI character.
      // There would be no one to actually give out a clue, if the clue spawner generated one.
      _clueSpawnerComponent->Deactivate();
   }

   Super::AuthorityNotSpawn(spawnContext, randomStream);
}

void ATATCharacterSpawner::AuthorityOnActorSpawnedDeferred_Implementation(const FTATVariationSpawnContext& spawnContext,
   AActor* actor, const FRandomStream& randomStream)
{
   Super::AuthorityOnActorSpawnedDeferred_Implementation(spawnContext, actor, randomStream);
   TryApplyModifiers(_modifiers, *actor, randomStream, false);
}

void ATATCharacterSpawner::GetSimpleCollisionCylinder(float& collisionRadius, float& collisionHalfHeight) const
{
   if (RootComponent == _capsuleComponent && IsRootComponentCollisionRegistered())
   {
      // Note: assuming vertical orientation
      _capsuleComponent->GetScaledCapsuleSize(collisionRadius, collisionHalfHeight);
   }
   else
   {
      Super::GetSimpleCollisionCylinder(collisionRadius, collisionHalfHeight);
   }
}

void ATATCharacterSpawner::_ValidateCollision()
{   
   FVector origLocation = GetActorLocation();
   const float radius = _capsuleComponent->GetScaledCapsuleRadius();
   FVector const slice(radius, radius, 1.f);

   bool result = true;

   // Check for adjustment
   FHitResult hit(ForceInit);
   const FVector traceStart = GetActorLocation();
   const FVector traceEnd = GetActorLocation() - FVector(0.f, 0.f, 4.f * _capsuleComponent->GetScaledCapsuleHalfHeight());
   GetWorld()->SweepSingleByChannel(hit, traceStart, traceEnd, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeBox(slice), FCollisionQueryParams(SCENE_QUERY_STAT(TATCharacterSpawner_Validate), false, this));
   if (hit.bBlockingHit)
   {
      const FVector hitLocation = traceStart + (traceEnd - traceStart) * hit.Time;
      FVector dest = hitLocation + FVector(0.f, 0.f, _capsuleComponent->GetScaledCapsuleHalfHeight() - 2.f);

      // Move actor (TEST ONLY) to see if navigation point moves
      TeleportTo(dest, GetActorRotation(), false, true);

      // If only adjustment was down tatards the floor, then it is a valid placement
      FVector newLocation = GetActorLocation();
      result = (newLocation.X == origLocation.X &&
         newLocation.Y == origLocation.Y &&
         newLocation.Z <= origLocation.Z);

      // Move actor back to original position
      TeleportTo(origLocation, GetActorRotation(), false, true);
   }

   // Update sprites by result
   _goodSprite->SetVisibility(result);
   _badSprite->SetVisibility(!result);
}

UShapeComponent* ATATCharacterSpawner::_GetShapeComponent() const
{
   return _capsuleComponent;
}

