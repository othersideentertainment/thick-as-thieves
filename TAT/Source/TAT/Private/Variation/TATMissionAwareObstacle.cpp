// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/TATMissionAwareObstacle.h"

//tat
#include "Variation/TATSpawnerComponent.h"

// ose

// ue4
#include "Engine/SCS_Node.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMissionAwareObstacle)

ATATMissionAwareObstacle::ATATMissionAwareObstacle()
   : Super()
{
   PrimaryActorTick.bCanEverTick = false;
   PrimaryActorTick.bStartWithTickEnabled = false;
   bReplicates = true; // so we can destroy ourselves when we're not chosen for a mission and have that replicated to clients

   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
   RootComponent->Mobility = EComponentMobility::Stationary;
   RootComponent->SetCanEverAffectNavigation(false);

   _spawnerComponent = CreateDefaultSubobject<UTATSpawnerComponent>(TEXT("VariationSpawner"));
}

void ATATMissionAwareObstacle::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   if (_spawnerComponent->GetSpawnType() != ETATSpawnChanceType::Disabled)
   {
      _spawnerComponent->AuthorityOnSpawn.AddUniqueDynamic(this, &ATATMissionAwareObstacle::_AuthorityOnMissionSpawnerSpawn);
      _spawnerComponent->AuthorityOnNotSpawn.AddUniqueDynamic(this, &ATATMissionAwareObstacle::_AuthorityOnMissionSpawnerNotSpawn);
   }
   else
   {
      // stop replicating if we're set to disabled
      SetReplicates(false);
   }
}

#if WITH_EDITOR
void ATATMissionAwareObstacle::CheckForErrors()
{
   // CheckForErrors is called when saving the asset but only on the CDO, so we can't check components added in blueprints.
   // HOWEVER it is also called for MapCheck which is what we're relying on here to gather up warnings

   Super::CheckForErrors();

   // obstacles should not have static or moveable static mesh components inside of them
   TInlineComponentArray<UActorComponent*> staticMeshComponents;
   GetComponents(UStaticMeshComponent::StaticClass(), staticMeshComponents);
   for (UActorComponent* staticMeshActorComp : staticMeshComponents)
   {
      if (UStaticMeshComponent* staticMeshComp = Cast<UStaticMeshComponent>(staticMeshActorComp))
      {
         if (staticMeshComp->Mobility != EComponentMobility::Stationary)
         {
            FMessageLog("MapCheck").Warning()
               ->AddToken(FUObjectToken::Create(this))
               ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("[TATMissionAwareObstacle] %s is not set to stationary mobility!"), *staticMeshComp->GetReadableName()))));
         }

         if (staticMeshComp->CanEverAffectNavigation())
         {
            FMessageLog("MapCheck").Warning()
               ->AddToken(FUObjectToken::Create(this))
               ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(TEXT("[TATMissionAwareObstacle] %s has \"Can Ever Affect Navigation\" set to true but should be false!"), *staticMeshComp->GetReadableName()))));         
         }
      }
   }
}
#endif // WITH_EDITOR

void ATATMissionAwareObstacle::_AuthorityOnMissionSpawnerSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   // stop replicating if we're staying in the level; we only needed that for the destruction case
   SetReplicates(false);
}

void ATATMissionAwareObstacle::_AuthorityOnMissionSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream)
{
   // destroy ourselves if the mission system decided not to spawn us
   Destroy();
}

