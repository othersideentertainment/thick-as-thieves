// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Utl/OSEShapeCollisionTrackerComponent.h"

// ose

// ue4
#include "Algo/AnyOf.h"
#include "Components/ShapeComponent.h"
#include "Engine/OverlapInfo.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEShapeCollisionTrackerComponent)

DEFINE_LOG_CATEGORY_STATIC(LogOSEShaopeCollisionTrackerComponent, Log, All);

#define CHECK_SHAPETRACKER_TICK_INVARIANT() check(IsComponentTickEnabled() == Algo::AnyOf(_actorOverlapState, [this](const auto& entry) { return entry.Value.OverlapDuration < RequiredTimeInsideOfShape; }))

UOSEShapeCollisionTrackerComponent::UOSEShapeCollisionTrackerComponent()
   : Super()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bAllowTickOnDedicatedServer = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;

   bWantsInitializeComponent = true;
}

void UOSEShapeCollisionTrackerComponent::InitializeComponent()
{
   Super::InitializeComponent();
   
   if (_IsEnabled())
   {
      if (AutomaticallyFindShapeComponent)
      {
         const TInlineComponentArray<UShapeComponent*> shapeComps(GetOwner());
         if (shapeComps.Num() > 0)
         {
            const TArrayView<UShapeComponent* const> arrayView = MakeArrayView(shapeComps.GetData(), shapeComps.Num());
            _SetShapeComponents(arrayView);
         }
      }
   }
}

void UOSEShapeCollisionTrackerComponent::BeginPlay()
{
   Super::BeginPlay();

   // NOTE: If a player pawn spawns in the world during BeginPlay while overlapping us, the overlap will be registered by Unreal.
   // However, it will not send a callback to us via OnComponentBeginOverlap since those are not done during BeginPlay
   // To get around this, on the first frame after BeginPlay, we check if any of our components are overlapping any actors
   // that we are not tracking via _actorOverlapState: this ensures that we have a consistent overlap state.
   //
   // There is the bool `bGenerateOverlapEventsDuringLevelStreaming` that can let some actors receive overlap callbacks during BeginPlay,
   // but this is not relevant for player pawns since they are not considered part of level streaming
   GetWorld()->GetTimerManager().SetTimerForNextTick (this, &UOSEShapeCollisionTrackerComponent::_HandleInitialOverlaps);
}

void UOSEShapeCollisionTrackerComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);

   if (_IsEnabled())
   {
      ClearShapeTracking();
   }
}

void UOSEShapeCollisionTrackerComponent::_HandleInitialOverlaps()
{
   // If we're already tracking an actor in _actorOverlapState, then it's been registered in _OnShapeBeginOverlap
   // and we shouldn't deal with it here (to avoid double-counting)
   TSet<AActor*> actorsAlreadyInOverlapState;
   for (const auto& existingOverlapState : _actorOverlapState)
   {
      actorsAlreadyInOverlapState.Add(existingOverlapState.Key);
   }

   for (UShapeComponent* shapeComponent : _shapeComponents)
   {
      // If an actor is in OverlapInfos, make sure we count if
      // It will notify us on exit via _OnShapeEndOverlap
      for (const FOverlapInfo& overlapInfo : shapeComponent->GetOverlapInfos())
      {
         if (AActor* overlappingActor = overlapInfo.OverlapInfo.GetActor())
         {
            if (!actorsAlreadyInOverlapState.Contains(overlappingActor))
            {
               if (!IgnoreOwner || (overlappingActor != GetOwner()))
               {
                  _HandleBeginOverlapWithActor(overlappingActor);
               }
            }
         }
      }
   }
}

void UOSEShapeCollisionTrackerComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   if (ensure(_IsEnabled()))
   {
      // avoid the broadcast while iterating because it may change our container
      TArray<AActor*> newActorsInShape;
      
      for (auto& entry : _actorOverlapState)
      {
         float& overlapDuration = entry.Value.OverlapDuration;
         if (overlapDuration < RequiredTimeInsideOfShape)
         {
            overlapDuration += deltaTime;

            if (overlapDuration >= RequiredTimeInsideOfShape)
            {
               newActorsInShape.Add(entry.Key);
            }
         }
      }

      for (AActor* newActorInShape : newActorsInShape)
      {
         OnActorEnteredShape.Broadcast(newActorInShape);
      }

      // Note: This induces a second iteration of the map - but OnActorEnteredShape _should_ cause the collider to move which in turn adds new overlap states to _actorOverlapState.
      const bool needsToTick = Algo::AnyOf(_actorOverlapState, [this](const auto& entry) { return entry.Value.OverlapDuration < RequiredTimeInsideOfShape; });
      if (needsToTick == false)
      {
         SetComponentTickEnabled(false);
      }

      CHECK_SHAPETRACKER_TICK_INVARIANT();
   }
}

void UOSEShapeCollisionTrackerComponent::ClearShapeTracking()
{
   // Unbind from shapes
   for (UShapeComponent* shapeComponent : _shapeComponents)
   {
      if (IsValid(shapeComponent))
      {
         shapeComponent->OnComponentBeginOverlap.RemoveAll(this);
         shapeComponent->OnComponentEndOverlap.RemoveAll(this);
      }
   }

   // if we have anyone who is already overlapped, make sure they are exited.
   for (const TTuple<AActor*, FOSEShapeCollisionTrackerOverlapData>& currentOverlaps : _actorOverlapState)
   {
      const float timeInShape = currentOverlaps.Value.OverlapDuration;
      if ((timeInShape) >= RequiredTimeInsideOfShape)
      {
         OnActorExitedShape.Broadcast(currentOverlaps.Key);
      }
   }
   
   // Clear tracked shapes and overlap state
   _shapeComponents.Reset();
   _actorOverlapState.Empty();
   SetComponentTickEnabled(false);
   
   CHECK_SHAPETRACKER_TICK_INVARIANT();
}

void UOSEShapeCollisionTrackerComponent::SetShapeComponent(UShapeComponent* shapeComponent)
{
   if (IsValid(shapeComponent))
   {
      _SetShapeComponents(MakeArrayView(&shapeComponent, 1));
   }
   else
   {
      ClearShapeTracking();
   }
}

void UOSEShapeCollisionTrackerComponent::RefreshInitialOverlaps()
{
   _HandleInitialOverlaps();
}

void UOSEShapeCollisionTrackerComponent::SetShapeComponents(const TArray<UShapeComponent*>& shapeComponents)
{
   _SetShapeComponents(shapeComponents);
}

void UOSEShapeCollisionTrackerComponent::_SetShapeComponents(const TArrayView<UShapeComponent* const>& shapeComponents)
{
   if (!_IsEnabled())
      return;

   // unbind from previous shapes
   ClearShapeTracking();

   // rebind to new shapes
   _shapeComponents = shapeComponents;
   for (UShapeComponent* shape : _shapeComponents)
   {
      check(IsValid(shape));
      shape->OnComponentBeginOverlap.AddUniqueDynamic(this, &UOSEShapeCollisionTrackerComponent::_OnShapeBeginOverlap);
      shape->OnComponentEndOverlap.AddUniqueDynamic(this, &UOSEShapeCollisionTrackerComponent::_OnShapeEndOverlap);
   }
}

TArray<AActor*> UOSEShapeCollisionTrackerComponent::GetOverlappedActors() const
{
   TArray<AActor*> overlappedActorKeys;
   _actorOverlapState.GetKeys(overlappedActorKeys);
   return overlappedActorKeys;
}

void UOSEShapeCollisionTrackerComponent::_HandleBeginOverlapWithActor(AActor* otherActor)
{
   if (FOSEShapeCollisionTrackerOverlapData* overlapData = _actorOverlapState.Find(otherActor))
   {
      // Increment overlapped shape count (deliberately avoid clamping against shape count, since other actor could have multiple components that fire the overlap)
      overlapData->OverlapCount++;
   }
   else
   {
      // Initialize overlap data
      _actorOverlapState.Add(otherActor, FOSEShapeCollisionTrackerOverlapData(0.f, 1));

      // broadcast for when we don't require any time inside the shape
      if (RequiredTimeInsideOfShape == 0.0f)
      {
         OnActorEnteredShape.Broadcast(otherActor);
      }
      else
      {
         SetComponentTickEnabled(true);
      }
   }
}

void UOSEShapeCollisionTrackerComponent::_OnShapeBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
   const bool isValidActor = otherActor && (!IgnoreOwner || (IgnoreOwner && otherActor != GetOwner()));
   if (isValidActor)
   {
      _HandleBeginOverlapWithActor(otherActor);

      UE_LOG(LogOSEShaopeCollisionTrackerComponent, Verbose, TEXT("%s ShapeBeginOverlap | %s overlapping with %d shapes")
         , *GetOwner()->GetName()
         , *otherActor->GetName()
         , _actorOverlapState[otherActor].OverlapCount);
   }
}

void UOSEShapeCollisionTrackerComponent::_OnShapeEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex)
{
   const bool isValidActor = otherActor && (!IgnoreOwner || (IgnoreOwner && otherActor != GetOwner()));
   if (isValidActor)
   {
      if(FOSEShapeCollisionTrackerOverlapData* overlapData = _actorOverlapState.Find(otherActor))
      {
         // Decrement overlap count
         overlapData->OverlapCount = FMath::Max(overlapData->OverlapCount - 1, 0);

         UE_LOG(LogOSEShaopeCollisionTrackerComponent, Verbose, TEXT("%s ShapeEndOverlap | %s overlapping with %d shapes")
            , *GetOwner()->GetName()
            , *otherActor->GetName()
            , overlapData->OverlapCount);

         // If actor no longer overlapping with any shapes
         if (overlapData->OverlapCount == 0)
         {
            const float timeInShape = overlapData->OverlapDuration;
            if ((timeInShape) >= RequiredTimeInsideOfShape)
            {
               OnActorExitedShape.Broadcast(otherActor);
            }

            _actorOverlapState.Remove(otherActor);
         }
         // let tick get removed in the next tick
      }
      // Aggressively broadcast for untracked actors (ex. actor overlap triggers before InitializeComponent() can bind).
      // Enabling bGenerateOverlapEventsDuringLevelStreaming on owning actor will help avoid situations like this
      else
      {
         UE_LOG(LogOSEShaopeCollisionTrackerComponent, Warning, TEXT("%s ShapeEndOverlap | Untracked actor %s triggered EndOverlap! Cannot determine accurate shape overlap count")
            , *GetOwner()->GetName()
            , *otherActor->GetName());

         OnActorExitedShape.Broadcast(otherActor);
      }
   }
}

bool UOSEShapeCollisionTrackerComponent::_IsEnabled() const
{
   return (OnlyRunOnAuthority && GetOwner()->HasAuthority()) || !OnlyRunOnAuthority;
}

#undef CHECK_SHAPETRACKER_TICK_INVARIANT

