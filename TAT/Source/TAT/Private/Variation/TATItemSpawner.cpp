// (c) 2021-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/TATItemSpawner.h"

// tat
#include "GameFramework/TATWorldSettings.h"
#include "Loot/TATLootActor.h"
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/TATSpawnData.h"
#include "Variation/TATSpawnerComponent.h"
#include "Variation/SceneVariants/TATSceneVariantUtils.h"
#include "Online/TATGameState.h"

// ue5
#include "Components/BillboardComponent.h"
#include "Components/BoxComponent.h"
#include "Engine/CollisionProfile.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATItemSpawner)

DEFINE_LOG_CATEGORY_STATIC(LogTATItemSpawner, Log, All);

ATATItemSpawner::ATATItemSpawner()
   : Super()
{   
   FVector boxExtent = FVector(20.0f, 20.0f, 20.0f);
   _boxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
   _boxComponent->ShapeColor = FColor(255, 138, 5, 255);
   _boxComponent->bDrawOnlyIfSelected = true;
   _boxComponent->InitBoxExtent(boxExtent);
   _boxComponent->SetRelativeLocation(FVector(0.0f, 0.0f, boxExtent.Z));
   _boxComponent->SetCollisionProfileName(UCollisionProfile::NoCollision_ProfileName);
   _boxComponent->bShouldCollideWhenPlacing = true;
   _boxComponent->SetShouldUpdatePhysicsVolume(true);
   _boxComponent->Mobility = EComponentMobility::Static;
   _boxComponent->SetupAttachment(RootComponent);

   // reparent the good/bad sprite to the box so it's more easily visible when we're sitting flat on something
   if (_goodSprite)
   {
      _goodSprite->SetupAttachment(_boxComponent);
   }
   if (_badSprite)
   {
      _badSprite->SetupAttachment(_boxComponent);
   }
}

void ATATItemSpawner::_ValidateCollision()
{
   FVector origLocation = GetActorLocation();
   FVector boxExtents = _boxComponent->GetUnscaledBoxExtent();

   FCollisionQueryParams params = FCollisionQueryParams(SCENE_QUERY_STAT(TATItemSpawner_Validate), false, this);

   // Validation for items just ensures that we don't have any blocking geo immediately on top of us for a few cm.  It's a fairly imprecise test...
   FHitResult hit(ForceInit);
   const FVector traceStart = GetActorLocation() + FVector(0.f, 0.f, 2.0f);
   const FVector traceEnd = traceStart + FVector(0.f, 0.f, 5.0f);
   GetWorld()->LineTraceSingleByChannel(hit, traceStart, traceEnd, ECC_Pawn, FCollisionQueryParams(SCENE_QUERY_STAT(TATItemSpawner_Validate), false, this));
   bool result = !hit.bBlockingHit;

   // Update sprites by result
   _goodSprite->SetVisibility(result);
   _badSprite->SetVisibility(!result);
}

UShapeComponent* ATATItemSpawner::_GetShapeComponent() const
{
   return _boxComponent;
}

void ATATItemSpawner::AuthorityOnActorSpawnFinished_Implementation(const FTATVariationSpawnContext& spawnContext, AActor* actor, const FRandomStream& randomStream)
{
   if (_triggerEndgameOnPickup && UTATSceneVariantUtils::ResolveBoolRequirement(GetWorld(), _triggerEndgameSceneRequirement))
   {
      if (ATATLootActor* lootActor = Cast<ATATLootActor>(actor))
      {
         lootActor->AuthorityOnPickedUp.AddLambda([](const ATATLootActor* lootActor, ACharacter* interactingCharacter)
            {
               if (ATATGameState* gameState = lootActor->GetWorld()->GetGameState<ATATGameState>())
               {
                  gameState->AuthorityStartEndgame(ETATEndgameReason::Mission);
               }
            });
      }
      else
      {
         UE_LOG(LogTATMapVariation, Warning, TEXT("Cannot bind to pickup for non-loot actor %s (in ItemSpawner %s)"), *GetNameSafe(actor), *GetActorNameOrLabel());
      }
   }
}

