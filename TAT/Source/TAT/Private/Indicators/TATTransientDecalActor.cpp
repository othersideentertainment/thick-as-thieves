// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Indicators/TATTransientDecalActor.h"

// ue
#include "Components/DecalComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTransientDecalActor)
DEFINE_LOG_CATEGORY_STATIC(LogTATTransientDecalActor, Log, All);

ATATTransientDecalActor::ATATTransientDecalActor()
{
   // We'll make a root scene component so the decal can be independently rotated and scaled
   _rootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("DecalRootComponent"));
   SetRootComponent(_rootComponent);

   _decalComponent = CreateDefaultSubobject<UDecalComponent>(TEXT("DecalComponent"));
   _decalComponent->SetupAttachment(_rootComponent);

   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = false;
}

void ATATTransientDecalActor::BeginPlay()
{
   Super::BeginPlay();

   if (_worldTimeAtSpawnOrRefresh == 0)
   {
      _worldTimeAtSpawnOrRefresh = GetWorld()->GetTimeSeconds();
   }

   if (_enableDirectionHint)
   {
      // Compute the start and end points
      _directionHintStart = GetActorLocation();
      _directionHintEnd = _directionHintStart + (GetActorRotation().Quaternion().GetForwardVector() * _directionHintDistance);

      // Only enable if there's actual distance to travel
      _enableDirectionHint = (_directionHintMovementDurationSeconds > 0 && FVector::Dist(_directionHintStart, _directionHintEnd) > 0.05f);
   }

   if (_autoUpdateMaximumOpacity)
   {
      SetAutoUpdateMaximumOpacity(true);
   }
}

void ATATTransientDecalActor::Tick(float deltaSeconds)
{
   Super::Tick(deltaSeconds);

   // Resetting the direction hint means quickly fading out, then teleporting back to the direction hint start pos.
   // When this is enabled, we suspend the normal opacity/movement behavior.
   if (_enableDirectionHint && _directionHintIsResetting)
   {
      const float opacityDelta = deltaSeconds * (1.0f / FMath::Max(_opacityFadeOutTimeSeconds, 0.1f));
      if (_currentOpacity > 0)
      {
         // Scale the delta by the maximum opacity so the fade out time is the same regardless of the maximum opacity
         _SetOpacityDirect(_currentOpacity - (opacityDelta * _maximumOpacity));
      }
      else
      {
         // done fading out. teleport back to the beginning
         _SetDirectionHintDirect(0.0f);
         _directionHintIsResetting = false;
      }
      return;
   }

   // Interpolate actor position to hint at a direction
   if (_enableDirectionHint && _currentDirectionHintNormalizedPos != _targetDirectionHintNormalizedPos)
   {
      auto updateDirectionHint = [this](float delta)
      {
         const float newPos = FMath::Clamp(_currentDirectionHintNormalizedPos + delta, 0.0f, 1.0f);
         if (_currentDirectionHintNormalizedPos != newPos)
         {
            _SetDirectionHintDirect(newPos);
         }
      };

      if (_targetDirectionHintNormalizedPos > _currentDirectionHintNormalizedPos)
      {
         updateDirectionHint(deltaSeconds * (1.0f / FMath::Max(_directionHintMovementDurationSeconds, 0.1f)));
      }
      else if (_targetDirectionHintNormalizedPos < _currentDirectionHintNormalizedPos)
      {
         updateDirectionHint(-1.0f * deltaSeconds * (1.0f / FMath::Max(_directionHintMovementDurationSeconds, 0.1f)));
      }
   }

   // Interpolate decal opacity when fading in and out
   if (_currentOpacity != _targetOpacity)
   {
      auto updateOpacity = [this](float delta)
      {
         // Scale the delta by the maximum opacity so we still get the same fade in/out times when the max opacity is less than 1.0
         const float newOpacity = FMath::Clamp(_currentOpacity + (delta * _maximumOpacity), 0.0f, _maximumOpacity);
         if (newOpacity != _currentOpacity)
         {
            _SetOpacityDirect(newOpacity);
         }
      };

      if (_currentOpacity < _targetOpacity)
      {
         updateOpacity(deltaSeconds * (1.0f / FMath::Max(_opacityFadeInTimeSeconds, 0.1f)));
      }
      else if (_currentOpacity > _targetOpacity)
      {
         updateOpacity(deltaSeconds * -1.0f * (1.0f / FMath::Max(_opacityFadeOutTimeSeconds, 0.1f)));
      }
   }
}

void ATATTransientDecalActor::OnSpawnedAsClientProxy_Implementation(float remainingLifeSpan, float indicatorLifeSpan, const FTATClientProxySpawnParams& spawnParams)
{
   _isClientProxy = true;

   if (remainingLifeSpan < 0 || remainingLifeSpan > indicatorLifeSpan)
   {
      constexpr float errorThreshold = 0.15f; // Only log an error if we're off by more than a little
      if ((remainingLifeSpan < 0 && FMath::Abs(remainingLifeSpan) > errorThreshold)
         || (remainingLifeSpan > 0 && FMath::Abs(remainingLifeSpan - indicatorLifeSpan) > errorThreshold))
      {
         UE_LOG(LogTATTransientDecalActor, Error, TEXT("Invalid remaining lifespan %.2f (with total lifespan %.2f). ")
            TEXT("Did you forget to call the parent function when overriding OnSpawnedAsClientProxy in actor '%s'?"),
            remainingLifeSpan, indicatorLifeSpan, *GetName());
      }
      remainingLifeSpan = FMath::Clamp(remainingLifeSpan, 0.0f, indicatorLifeSpan);
   }

   _worldTimeAtSpawnOrRefresh = GetWorld()->GetTimeSeconds();
   _clientProxyLifespanRemainingAtSpawnOrRefresh = remainingLifeSpan;
   _clientProxyLifespan = indicatorLifeSpan;
}

void ATATTransientDecalActor::OnDestroyClientProxy_Implementation()
{
   SetAutoUpdateMaximumOpacity(false);
   SetTargetOpacity(0.0f);
   SetLifeSpan(_opacityFadeOutTimeSeconds + 0.25f);
}

void ATATTransientDecalActor::OnClientProxyActorSetVisible_Implementation(bool newVisible)
{
   SetTargetOpacity(newVisible ? _maximumOpacity : 0.0f);
}

void ATATTransientDecalActor::OnClientProxyActorLifeSpanRefreshed_Implementation(float newRemainingLifeSpan)
{
   _worldTimeAtSpawnOrRefresh = GetWorld()->GetTimeSeconds();
   _clientProxyLifespanRemainingAtSpawnOrRefresh = newRemainingLifeSpan;

   if (_enableDirectionHint)
   {
      // Reset the target direction hint back to zero so the decal will interpolate back to the start and then move forward again
      _directionHintIsResetting = true;
   }
}

void ATATTransientDecalActor::SetTargetOpacity(float newTargetOpacity)
{
   _targetOpacity = FMath::Clamp(newTargetOpacity, 0.0f, _maximumOpacity);

   // reenable tick if needed
   if (!IsActorTickEnabled() && _targetOpacity != _currentOpacity)
   {
      SetActorTickEnabled(true);
   }
}

void ATATTransientDecalActor::SetMaximumOpacity(float newMaximumOpacity)
{
   // We use the maximum opacity as a multiplier to keep fade times consistent, so the value _must_ be greater than zero and less than or equal to 1.0.
   _maximumOpacity = FMath::Clamp(newMaximumOpacity, 0.001f, 1.0f);

   if (_targetOpacity > _maximumOpacity)
   {
      SetTargetOpacity(_maximumOpacity);
   }
}

UMaterialInstanceDynamic* ATATTransientDecalActor::GetOrCreateDecalMaterialInstanceDynamic()
{
   if (_decalMaterial == nullptr)
   {
      _decalMaterial = _ConstructDynamicMaterialInstance();
   }
   return _decalMaterial;
}

bool ATATTransientDecalActor::GetRemainingAndTotalLifeSpan(float& outRemainingLifeSpan, float& outTotalLifeSpan) const
{
   const float lifespan = _isClientProxy ? _clientProxyLifespan : InitialLifeSpan;
   if (lifespan <= 0)
   {
      outRemainingLifeSpan = 0.0f;
      outTotalLifeSpan = 0.0f;
      return false;
   }

   const float lifespanRemaining = _isClientProxy ? _clientProxyLifespanRemainingAtSpawnOrRefresh : GetLifeSpan();
   const float timeSinceSpawnOrRefresh = (_worldTimeAtSpawnOrRefresh > 0) ? (GetWorld()->GetTimeSeconds() - _worldTimeAtSpawnOrRefresh) : 0.0f;
   outRemainingLifeSpan = FMath::Clamp(lifespanRemaining - timeSinceSpawnOrRefresh, 0.0f, lifespan);
   outTotalLifeSpan = lifespan;
   return true;
}

bool ATATTransientDecalActor::SetAutoUpdateMaximumOpacity(bool enabled)
{
   if (!enabled)
   {
      if (_autoUpdateMaximumOpacityTimer.IsValid())
      {
         GetWorldTimerManager().ClearTimer(_autoUpdateMaximumOpacityTimer);
         return true;
      }
      return false;
   }

   float remainingLifeSpan = 0.0f;
   float totalLifeSpan = 0.0f;
   if (!GetRemainingAndTotalLifeSpan(remainingLifeSpan, totalLifeSpan))
   {
      // Can't auto-update over the lifespan if we have no lifespan
      return false;
   }

   // Update immediately before the timer is set
   _OnAutoMaxOpacityUpdate();

   constexpr bool looping = true;
   GetWorldTimerManager().SetTimer(_autoUpdateMaximumOpacityTimer, this, &ATATTransientDecalActor::_OnAutoMaxOpacityUpdate, _autoUpdateMaximumOpacityUpdateRate, looping);

   return true;
}

void ATATTransientDecalActor::_OnAutoMaxOpacityUpdate()
{
   float remainingLifeSpan = 0.0f;
   float totalLifeSpan = 0.0f;
   const bool haveLifeSpan = GetRemainingAndTotalLifeSpan(remainingLifeSpan, totalLifeSpan);
   check(haveLifeSpan);
   check(totalLifeSpan > 0);
   const float normalizedLifeSpan = remainingLifeSpan / totalLifeSpan;
   SetMaximumOpacity(FMath::GetMappedRangeValueClamped(_autoUpdateMaximumOpacityNormalizedLifespanRange, _autoUpdateMaximumOpacityOpacityRange, normalizedLifeSpan));
}

void ATATTransientDecalActor::_SetOpacityDirect(float newOpacity)
{
   UMaterialInstanceDynamic* material = GetOrCreateDecalMaterialInstanceDynamic();
   check(material != nullptr);
   _currentOpacity = FMath::Clamp(newOpacity, 0.0f, _maximumOpacity);
   material->SetScalarParameterValue(_materialOpacityParameterName, _currentOpacity);
   _OnOpacityChanged(_currentOpacity);
}

void ATATTransientDecalActor::_SetDirectionHintDirect(float newNormalizedPos)
{
   _currentDirectionHintNormalizedPos = FMath::Clamp(newNormalizedPos, 0.0f, 1.0f);
   const FVector newWorldLocation = FMath::Lerp(_directionHintStart, _directionHintEnd, _currentDirectionHintNormalizedPos);
   switch (_directionHintMovementMode)
   {
   case ETATTransientDecalDirectionHintMode::Custom:
      // Don't move any components - let the event handler take care of it.
      break;
   case ETATTransientDecalDirectionHintMode::MoveActor:
      SetActorLocation(newWorldLocation);
      break;
   case ETATTransientDecalDirectionHintMode::MoveDecalComponent:
      _decalComponent->SetWorldLocation(newWorldLocation);
      break;
   }
   _OnDirectionHintChanged(_currentDirectionHintNormalizedPos, newWorldLocation);
}

void ATATTransientDecalActor::_OnOpacityChanged(float newOpacity)
{
   const bool tickEnabled = IsActorTickEnabled();
   if (tickEnabled && newOpacity <= 0 && !_directionHintIsResetting)
   {
      // If we're invisible, stop ticking.
      // This means we'll stop updating the direction hint, but that's ok because we're not visible anyway.
      SetActorTickEnabled(false);
   }
   else if (!tickEnabled && newOpacity > 0)
   {
      SetActorTickEnabled(true);
   }

   OnOpacityChanged.Broadcast(newOpacity);
}

void ATATTransientDecalActor::_OnDirectionHintChanged(float newNormalizedPos, const FVector& newWorldLocation)
{
   if (_directionHintAutoReset && newNormalizedPos == 1)
   {
      _directionHintIsResetting = true;
   }

   OnDirectionHintChanged.Broadcast(newNormalizedPos, newWorldLocation);
}

UMaterialInstanceDynamic* ATATTransientDecalActor::_ConstructDynamicMaterialInstance()
{
   checkf(_decalMaterial == nullptr, TEXT("Dynamic decal material already created!"));

   check(_decalComponent != nullptr);
   if (_decalComponent->GetDecalMaterial() == nullptr)
   {
      UE_LOG(LogTATTransientDecalActor, Warning, TEXT("[%s] Unassigned decal material! Could not construct dynamic material instance"), *GetOwner()->GetName());
      return nullptr;
   }

   UMaterialInstanceDynamic* dynamicMaterialInstance = _decalComponent->CreateDynamicMaterialInstance();
   check(dynamicMaterialInstance != nullptr);
   dynamicMaterialInstance->SetScalarParameterValue(_materialOpacityParameterName, _currentOpacity);

   return dynamicMaterialInstance;
}
