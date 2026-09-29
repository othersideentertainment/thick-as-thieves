// (c) 2018-2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/Spawn/TATQuestSpawnIndicator.h"

// tat
#include "Online/TATGameState.h"
#include "Quests/Spawn/TATQuestActorSpawner.h"
#include "Variation/TATMapVariationMgrComponent.h"
#include "Variation/Clues/TATLocalClueFactSubsystem.h"
#include "Variation/SceneVariants/TATSceneVariantUtils.h"
#include "WorldMap/TATMapActorComponent.h"

// ue
#include "Misc/UObjectToken.h"
#include "Net/Core/PushModel/PushModel.h"
#include "Net/UnrealNetwork.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestSpawnIndicator)

// Sets default values
ATATQuestSpawnIndicator::ATATQuestSpawnIndicator()
{
   PrimaryActorTick.bCanEverTick = false;
   _mapComponent = CreateDefaultSubobject<UTATMapActorComponent>(TEXT("MapComponent"));
   bReplicates = true;
   NetDormancy = DORM_Initial;
}

void ATATQuestSpawnIndicator::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   FDoRepLifetimeParams params;
   params.bIsPushBased = true;

   DOREPLIFETIME_WITH_PARAMS_FAST(ThisClass, _factNamespace, params);
   DOREPLIFETIME_CONDITION(ThisClass, _clueFactTag, COND_InitialOnly);
}

#if WITH_EDITOR
void ATATQuestSpawnIndicator::CheckForErrors()
{
   Super::CheckForErrors();

   if (_questSpawner == nullptr)
   {
      FMessageLog("MapCheck").Error()
         ->AddToken(FUObjectToken::Create(this))
         ->AddToken(FTextToken::Create(INVTEXT("QuestSpawner not set")));
   }

   if (!_clueFactTag.IsValid())
   {
      FMessageLog("MapCheck").Error()
         ->AddToken(FUObjectToken::Create(this))
         ->AddToken(FTextToken::Create(INVTEXT("ClueFactTag not set")));
   }
}
#endif

void ATATQuestSpawnIndicator::AuthorityInit(AActor* spawnedActor, UTATQuestActorSpawnerComponent* spawner)
{
   spawnedActor->OnDestroyed.AddUniqueDynamic(this, &ThisClass::_OnSpawnedActorDestroyed);
   _questSpawner = Cast<ATATQuestActorSpawner>(spawner->GetOwner());
}

// Called when the game starts or when spawned
void ATATQuestSpawnIndicator::BeginPlay()
{
   Super::BeginPlay();

   if(!UTATSceneVariantUtils::ResolveBoolRequirement(GetWorld(), _sceneRequirement))
   {
      return;
   }

   // NB: quest spawning should happen after begin play on authority
   if (_questSpawner && HasAuthority())
   {
      _questSpawner->GetQuestSpawner()->OnActorSpawned.AddWeakLambda(this, [this](AActor* spawnedActor)
      {
         check(spawnedActor);
         spawnedActor->OnDestroyed.AddUniqueDynamic(this, &ThisClass::_OnSpawnedActorDestroyed);
      });
   }

   if (_locationWillRandomize)
   {
      if (HasAuthority())
      {
         if (const ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>())
         {
            if (UTATMapVariationMgrComponent* mapVariationMgr = gameState->GetMapVariationMgr())
            {
               mapVariationMgr->AuthorityCallOrWaitForComplete(FSimpleDelegate::CreateUObject(this, &ThisClass::_AuthorityInitNamespace, mapVariationMgr));
            }
         }
      }
   }
   else if (!IsNetMode(NM_DedicatedServer))
   {
      if(UTATLocalClueFactSubsystem* factSubsystem = GetWorld()->GetSubsystem<UTATLocalClueFactSubsystem>())
      {
         // NOTE: explicitly not unregistering in EndPlay, because each callback is only called once, so the cost of registering
         //       would dominate any savings
         factSubsystem->CallOrRegisterFactDelegate(_clueFactTag, FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::_OnFactKnown));
      }
   }
}

void ATATQuestSpawnIndicator::_AuthorityInitNamespace(UTATMapVariationMgrComponent* variationMgr)
{
   if (!IsValid(_questSpawner))
   {
      return;
   }

   if (const FTATClueFactNamespace* factNamespace = variationMgr->AuthorityGetClueFactNamespacesByQuestSpawner().Find(_questSpawner->GetQuestSpawner()))
   {
      FlushNetDormancy();
      _factNamespace = *factNamespace;
      MARK_PROPERTY_DIRTY_FROM_NAME(ATATQuestSpawnIndicator, _factNamespace, this);
      if (!IsNetMode(NM_DedicatedServer))
      {
         _OnRep_FactNamespace();
      }
   }
}

void ATATQuestSpawnIndicator::_OnFactKnown()
{
   BP_OnFactKnown();
}


void ATATQuestSpawnIndicator::_OnSpawnedActorDestroyed(AActor* destroyedActor)
{
   Destroy();
}

void ATATQuestSpawnIndicator::_OnRep_FactNamespace()
{
   if(UTATLocalClueFactSubsystem* factSubsystem = GetWorld()->GetSubsystem<UTATLocalClueFactSubsystem>())
   {
      // NOTE: explicitly not unregistering in EndPlay, because each callback is only called once, so the cost of registering
      //       would dominate any savings
      factSubsystem->CallOrRegisterNamespacedFactDelegate(_clueFactTag, _factNamespace, FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &ThisClass::_OnFactKnown));
   }
}

void FTATQuestActorSpawnAction_Indicator::OnSpawnedQuestActor(AActor* spawnedActor, UTATQuestActorSpawnerComponent* spawner) const
{
   check(spawnedActor && spawner);

   UWorld* world = spawnedActor->GetWorld();
   check(world);

   if(!ensure(SpawnIndicatorClass.Get()))
   {
      return;
   }

   const AActor* resolvedLocationProxy = OptionalLocationProxy.Get();
   FTransform location = resolvedLocationProxy ? resolvedLocationProxy->GetTransform() : spawnedActor->GetTransform();

   if (ATATQuestSpawnIndicator* indicator = world->SpawnActorDeferred<ATATQuestSpawnIndicator>(SpawnIndicatorClass, location))
   {
      indicator->AuthorityInitFactTag(ClueFactTag);
      indicator->AuthorityInit(spawnedActor, spawner);
      
      indicator->FinishSpawning(location);
   }
}

#if WITH_EDITOR
void FTATQuestActorSpawnAction_Indicator::Validate(TFunctionRef<void(const FText&)> reportError) const
{
   if(!ClueFactTag.IsValid())
   {
      reportError(INVTEXT("No ClueFactTag"));
   }
   if(SpawnIndicatorClass == nullptr)
   {
      reportError(INVTEXT("No SpawnIndicatorClass"));
   }
}
#endif


