// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Environment/TATAreaMarkupVolume.h"

#include "Character/OSECharacterBase.h"
#include "Environment/TATAreaMarkupInterface.h"
#include "Interactables/Electrical/TATPowerSource.h"
#include "Online/TATGameState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAreaMarkupVolume)

namespace TATAreaMarkupVolume
{
   bool IsMapVariationLoadingStateIsValid(const ETATMapVariationLoadingState state)
   {
      return state == ETATMapVariationLoadingState::CompleteNoVariation || state == ETATMapVariationLoadingState::CompleteWithVariation;
   }
}

ATATAreaMarkupVolume::ATATAreaMarkupVolume()
{
   PrimaryActorTick.bCanEverTick = false;
   PrimaryActorTick.bStartWithTickEnabled = false;

   // this actor shouldn't exist on client
   bNetLoadOnClient = false;
}

void ATATAreaMarkupVolume::_HandleMapStateChanged(const ETATMapVariationLoadingState currentState)
{
   if(TATAreaMarkupVolume::IsMapVariationLoadingStateIsValid(currentState))
   {
      _WaitForWorldBegunPlayOrTrigger();
   }
}

void ATATAreaMarkupVolume::_WaitForWorldBegunPlayOrTrigger()
{
   if(UWorld* world = GetWorld())
   {
      if(world->HasBegunPlay())
      {
         _OnWorldBegunPlay();
      }
      else
      {
         world->OnWorldBeginPlay.AddUObject(this, &ThisClass::_OnWorldBegunPlay);
      }
   }
}

void ATATAreaMarkupVolume::BeginPlay()
{
   Super::BeginPlay();
   for (auto powerSource : _PowerSources)
   {
      if(powerSource.IsValid() == false)
         continue;
      powerSource->OnPowerStateChanged.AddUObject(this, &ThisClass::_HandlePowerStateChanged, powerSource);
   }
   _IsReadyToProcessActorOverlaps = false;
   
   bool bShouldBindOnWorldBeginPlay = true;
   if (const ATATGameState* gameState = Cast<ATATGameState>(GetWorld()->GetGameState()))
   {
      if(UTATMapVariationMgrComponent* mapVariationMgr = gameState->GetMapVariationMgr())
      {
         const ETATMapVariationLoadingState state = mapVariationMgr->GetCurrentMapVariationLoadingState();
         if(TATAreaMarkupVolume::IsMapVariationLoadingStateIsValid(state) == false)
         {
            mapVariationMgr->OnMapVariationMgrStateChanged.AddDynamic(this, &ThisClass::_HandleMapStateChanged);
         }
         else
         {
            _HandleMapStateChanged(state);            
         }
         bShouldBindOnWorldBeginPlay = false;
      }
   }
   
   if(bShouldBindOnWorldBeginPlay)
   {
      _WaitForWorldBegunPlayOrTrigger();
   }
}

void ATATAreaMarkupVolume::_HandleInitialOverlaps()
{
   TArray<AActor*> overlappingActors;
   GetOverlappingActors(overlappingActors, AOSECharacterBase::StaticClass());
   _IsReadyToProcessActorOverlaps = true;
   for (AActor* const overlappingActor : overlappingActors)
   {
      _HandleOverlapWithActor(overlappingActor);
   }
}

void ATATAreaMarkupVolume::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   for (auto powerSource : _PowerSources)
   {
      if(powerSource.IsValid() == false)
         continue;
      powerSource->OnPowerStateChanged.RemoveAll(this);
   }
   Super::EndPlay(endPlayReason);
}

void ATATAreaMarkupVolume::_OnWorldBegunPlay()
{
   if(UWorld* world = GetWorld())
   {
      world->OnWorldBeginPlay.RemoveAll(this);
   }
   _HandleInitialOverlaps();
}

void ATATAreaMarkupVolume::_HandlePowerStateChanged(const bool isToggledOn, const TWeakObjectPtr<ATATPowerSource> powerSource)
{
   OnAreaPowerSourceStatusChanged.Broadcast(isToggledOn, powerSource);
}

void ATATAreaMarkupVolume::_HandleOverlapWithActor(AActor* actor)
{
   if(_IsReadyToProcessActorOverlaps == false)
      return;
   if(ITATAreaMarkupInterface* areaMarkupInterface = Cast<ITATAreaMarkupInterface>(actor))
   {
      areaMarkupInterface->AddArea(this);
   }
}

void ATATAreaMarkupVolume::NotifyActorBeginOverlap(AActor* otherActor)
{
   Super::NotifyActorBeginOverlap(otherActor);
   _HandleOverlapWithActor(otherActor);
}

void ATATAreaMarkupVolume::NotifyActorEndOverlap(AActor* otherActor)
{
   if(ITATAreaMarkupInterface* areaMarkupInterface = Cast<ITATAreaMarkupInterface>(otherActor))
   {
      areaMarkupInterface->RemoveArea(this);
   }
   Super::NotifyActorEndOverlap(otherActor);
}
