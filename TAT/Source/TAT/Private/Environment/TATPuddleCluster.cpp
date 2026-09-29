// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATPuddleCluster.h"

// tat
#include "Environment/TATPuddleSubsystem.h"
#include "Environment/TATPuddleUtilities.h"
#include "Developer/TATWeatherSettings.h"
#include "Character/TATCharacterBase.h"
#include "Developer/TATImGuiHelpers.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"

// ue
#include "Components/CapsuleComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/DecalComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameStateBase.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPuddleCluster)

DEFINE_LOG_CATEGORY_STATIC(LogTATPuddleCluster, Log, All);

DECLARE_STATS_GROUP(TEXT("Puddle Cluster"), STATGROUP_PuddleCluster, STATCAT_Advanced);
DECLARE_CYCLE_STAT(TEXT("Puddle cluster cosmetic tick"), STAT_PuddleCluster_CosmeticTick, STATGROUP_PuddleCluster)
DECLARE_CYCLE_STAT(TEXT("Puddle cluster authority update"), STAT_PuddleCluster_AuthorityUpdate, STATGROUP_PuddleCluster)


namespace PuddleHelpers
{
   FORCEINLINE float GetPuddleLifeSpanCosmetic(float maxLifeSpan)
   {
      // Use a portion of the actual remaining lifespan so the fade out can complete _before_ the puddle completely goes away.
      // Use whichever version is larger: subtracting a small constant, a percentage of the value, or a very small constant value (mostly here to avoid dividing by zero).
      return FMath::Max3(maxLifeSpan - 0.25f, maxLifeSpan * 0.9f, 0.05f);
   }

   FORCEINLINE float GetNormalizedHealth(float currentHealth, float maxHealth)
   {
      return FMath::Clamp((maxHealth > 0) ? (currentHealth / maxHealth) : 0.0f, 0.0f, 1.0f);
   }

   float MapVelocityOntoValueRange(const FVector& velocity, const FFloatInterval& velocityRange, const FFloatInterval& valueRange, EOSEInterpMode interpMode, bool disableIfVelocityLessThanMinimum)
   {
      const float velocitySquared = velocity.SquaredLength();
      if (velocitySquared <= FMath::Square(velocityRange.Min))
      {
         return disableIfVelocityLessThanMinimum ? 0.0f : valueRange.Min;
      }
      if (velocitySquared >= FMath::Square(velocityRange.Max))
      {
         return valueRange.Max;
      }
      const double alpha = FMath::GetMappedRangeValueClamped(FVector2D(velocityRange.Min, velocityRange.Max), { 0.0, 1.0 }, FMath::Sqrt(velocitySquared));
      return UOSEMathFunctionLibrary::Interpolate(valueRange.Min, valueRange.Max, alpha, interpMode);
   }
}

float FTATPuddleMovementEffect::GetPuddleDamagePerSecond(const FVector& velocity) const
{
   return PuddleHelpers::MapVelocityOntoValueRange(velocity, TargetVelocityRange, PuddleDamage.PuddleDamagePerSecondRange, PuddleDamage.PuddleDamagePerSecondCurve, DisableIfVelocityLessThanMinimum);
}

float FTATPuddleMovementEffect::GetChancePerSecondToApplyGameplayEffect(const FVector& velocity) const
{
   return PuddleHelpers::MapVelocityOntoValueRange(velocity, TargetVelocityRange, GameplayEffect.ChanceToApplyPerSecondRange, GameplayEffect.ChanceToApplyCurve, DisableIfVelocityLessThanMinimum);
}

float FTATPuddleMovementEffect::GetChanceOnEnterToApplyGameplayEffect(const FVector& velocity) const
{
   return PuddleHelpers::MapVelocityOntoValueRange(velocity, TargetVelocityRange, GameplayEffect.ChanceToApplyOnEnter, GameplayEffect.ChanceToApplyCurve, DisableIfVelocityLessThanMinimum);
}

ATATPuddleCluster::ATATPuddleCluster()
{
   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("PuddleClusterRootComponent"));
   RootComponent->SetMobility(EComponentMobility::Movable);

   static const FName defaultPuddleCollisionProfile = TEXT("OverlapAllDynamic");
   PuddleCollisionProfile.Name = defaultPuddleCollisionProfile;

   bReplicates = true;
   NetDormancy = DORM_Initial;

   PrimaryActorTick.bCanEverTick = true;
   PrimaryActorTick.bStartWithTickEnabled = true;
   PrimaryActorTick.bAllowTickOnDedicatedServer = false;
}

void ATATPuddleCluster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATPuddleCluster, _puddleClusterPendingDestroy);
   DOREPLIFETIME(ATATPuddleCluster, _puddles);
}

void ATATPuddleCluster::OnConstruction(const FTransform& transform)
{
   Super::OnConstruction(transform);
}

void ATATPuddleCluster::BeginPlay()
{
   Super::BeginPlay();
   if (UTATPuddleSubsystem* puddleSubsystem = GetWorld()->GetSubsystem<UTATPuddleSubsystem>())
   {
      puddleSubsystem->RegisterPuddleCluster(this);
   }

   // Cache the outdoor puddle DPS so we don't have to keep looking up the current weather type
   const bool debugDrawPuddleTraces = EnumHasAnyFlags(PuddleHelpers::GetPuddleDebugDraw(), PuddleHelpers::EPuddleDebugDraw::OutsideTraces);
   bool isDefaultWeatherType = false;
   if (const float* outdoorPuddleDPS = OutsidePuddleHealthLossPerSecondFromWeather.Find(UTATWeatherSettings::GetCurrentWeatherType(this, isDefaultWeatherType)))
   {
      _outsidePuddleDamagePerSecond = *outdoorPuddleDPS;

      // Any puddles that were spawned before this actor finished spawning won't have their IsOutside value initialized correctly, so refresh those now
      if (_outsidePuddleDamagePerSecond > 0)
      {
         for (int32 puddleIndex = 0; puddleIndex < _puddles.Num(); puddleIndex++)
         {
            const FTATPuddle& puddle = _puddles[puddleIndex];
            FTATPuddleState& state = _puddleStates.FindChecked(puddle.PuddleId);
            state.IsOutside = (_outsidePuddleDamagePerSecond > 0) ? UTATPuddleUtilities::IsPuddleOutside(this, puddle, this, debugDrawPuddleTraces) : false;
         }
         _SyncPuddleStates();
      }
   }
   else
   {
      _outsidePuddleDamagePerSecond = 0.0f;
   }

   // Use a separate tick function on the server so it can poll at a much slower rate than normal tick
   if (HasAuthority())
   {
      constexpr bool looping = true;
      const float updateInterval = (PuddleHealthUpdatesPerSecond > 0) ? (1.0f / PuddleHealthUpdatesPerSecond) : 0.0f;
      FTimerHandle timerHandle;
      GetWorldTimerManager().SetTimer(timerHandle, FTimerDelegate::CreateUObject(this, &ATATPuddleCluster::_AuthorityUpdatePuddles), updateInterval, looping);
   }
}

void ATATPuddleCluster::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   // Clean up - remove all characters still in this cluster from puddles, firing blueprint events and removing gameplay effects if requested
   constexpr bool forceRemove = true;
   for (ACharacter* character : _charactersInCluster)
   {
      _TryRemoveCharacterFromCluster(character, forceRemove);
   }

   if (UTATPuddleSubsystem* puddleSubsystem = GetWorld()->GetSubsystem<UTATPuddleSubsystem>())
   {
      puddleSubsystem->UnregisterPuddleCluster(this);
   }

   Super::EndPlay(endPlayReason);
}

void ATATPuddleCluster::Tick(float deltaSeconds)
{
   Super::Tick(deltaSeconds);

   // We only use tick for cosmetic purposes
   check(GetNetMode() != NM_DedicatedServer);

   SCOPE_CYCLE_COUNTER(STAT_PuddleCluster_CosmeticTick);

   UWorld* world = GetWorld();
   check(world != nullptr);
   const AGameStateBase* gameState = world->GetGameState();
   check(gameState != nullptr);
   const double serverWorldTimeSeconds = gameState->GetServerWorldTimeSeconds();
   const double clientWorldTimeSeconds = GetWorld()->GetTimeSeconds();

   // Update cosmetic bits
   if (GetNetMode() != NM_DedicatedServer)
   {
      const float clusterActorLifeSpanRemaining = _puddleClusterPendingDestroy ? GetLifeSpan() : 0.0f;

      constexpr bool fromTick = true;
      for (const FTATPuddle& puddle : _puddles)
      {
         FTATPuddleState& state = _puddleStates.FindChecked(puddle.PuddleId);
         _UpdatePuddleState(state, puddle, fromTick, deltaSeconds);
      }
   }

#if TAT_ALLOW_PUDDLE_DEBUG
   // Puddle debug info
   const PuddleHelpers::EPuddleDebugDraw puddleDebugFlags = PuddleHelpers::GetPuddleDebugDraw();
   if (PuddleHelpers::IsPuddleDebugEnabled() && puddleDebugFlags != PuddleHelpers::EPuddleDebugDraw::Hidden)
   {
      constexpr int32 pointCount = 8;
      constexpr float extentMultiplier = 1.0f;
      constexpr float angleIncrementDeg = 360.0f / static_cast<float>(pointCount);

      for (const FTATPuddle& puddle : _puddles)
      {
         const FTATPuddleState* puddleState = _puddleStates.Find(puddle.PuddleId);
         const FQuat puddleOrientation = puddle.Rotation.Quaternion();

         if (EnumHasAnyFlags(puddleDebugFlags, PuddleHelpers::EPuddleDebugDraw::Radius))
         {
            const FVector lastPt = puddle.GetPositionAroundPuddle((pointCount - 1) * angleIncrementDeg, extentMultiplier);
            FVector prevPt = FVector::ZeroVector;
            for (int32 i = 0; i < pointCount; i++)
            {
               const FVector ptA = puddle.GetPositionAroundPuddle(i * angleIncrementDeg, extentMultiplier);
               const FVector& ptB = (i > 0) ? prevPt : lastPt;
               DrawDebugLine(world, ptA, ptB, PuddleColor.ToFColor(false), false, 0.0f);
               prevPt = ptA;
            }
         }

         if (EnumHasAnyFlags(puddleDebugFlags, PuddleHelpers::EPuddleDebugDraw::BoundingBox))
         {
            if (puddleState && puddleState->Collision)
            {
               const FVector boxOrigin = puddleState->Collision->GetComponentLocation();
               const FVector boxExtent = puddleState->Collision->GetScaledBoxExtent();
               const FQuat boxOrientation = puddleState->Collision->GetComponentQuat();
               DrawDebugBox(world, boxOrigin, boxExtent, boxOrientation, PuddleColor.ToFColor(false), false, 0.0f);
            }
         }

         // Draw any puddle state-related indicators
         if (EnumHasAnyFlags(puddleDebugFlags, PuddleHelpers::EPuddleDebugDraw::StateIndicator))
         {
            auto makeIndicatorLocation = [&](float offset) -> FVector
            {
               return puddle.Location + (puddleOrientation.GetUpVector() * (puddle.Extent.Z + offset));
            };

            constexpr float angleIndicatorSize = 20.0f;
            constexpr float angleIndicatorThickness = 1.5f;
            auto drawAngleIndicator = [&](const FVector& angleDirection, const FColor& color)
            {
               const FVector baseLoc = makeIndicatorLocation(3.0f);
               DrawDebugDirectionalArrow(world, baseLoc, baseLoc + (angleDirection * angleIndicatorSize), angleIndicatorSize * 0.5f, color, false, -1.0f, 0, angleIndicatorThickness);
            };

            switch (puddleState->SurfaceAngle)
            {
            case ETATPuddleSurfaceAngle::Floor:
               drawAngleIndicator(FVector::UpVector, FColor(100, 255, 255));
               break;
            case ETATPuddleSurfaceAngle::Ceiling:
               drawAngleIndicator(FVector::DownVector, FColor(255, 100, 255));
               break;
            case ETATPuddleSurfaceAngle::Wall:
               drawAngleIndicator(FVector::VectorPlaneProject(puddleOrientation.GetUpVector(), FVector::UpVector).GetSafeNormal(), FColor(100, 255, 100));
               break;
            case ETATPuddleSurfaceAngle::Angled:
               drawAngleIndicator(puddleOrientation.GetUpVector(), FColor(255, 100, 100));
               break;
            default:
               break;
            }

            // For outside puddles (that take weather-related damage), draw a cloud icon to show it's outside
            if (puddleState->IsOutside)
            {
               constexpr float iconSize = 28.0f;
               constexpr FColor cloudColor(255, 255, 255);
               constexpr FColor raindropColor(25, 130, 200);
               constexpr bool raindropsAsQuads = true;
               const FQuat orientation = FQuat(puddleOrientation.GetRightVector(), FMath::DegreesToRadians(90.0f)) * puddleOrientation;
               PuddleHelpers::DrawDebugCloudIcon(world, makeIndicatorLocation(5.0f), iconSize, cloudColor, raindropColor, raindropsAsQuads, orientation,
                  false, -1.0f, 0, 1.25f);
            }
         }

         // Draw the puddle's health bar
         if (EnumHasAnyFlags(puddleDebugFlags, PuddleHelpers::EPuddleDebugDraw::HealthBars))
         {
            // Use HSV to pick a health bar color that won't blend in with the puddles
            FLinearColor hsv = PuddleColor.LinearRGBToHSV();
            hsv.R += 45.0f; // hue (0..360)
            hsv.G = FMath::Max(hsv.G, 0.25f); // sat (0..1)
            hsv.B = FMath::Max(hsv.B, 0.8f); // val (0..1)
            const FColor healthBarValueColor = hsv.HSVToLinearRGB().ToFColorSRGB();
            constexpr FColor healthBarEmptyColor = FColor(180, 180, 180);
            constexpr float healthBarThickness = 0.0f;
            constexpr int32 healthBarNumSegments = 12;
            constexpr float healthBarRadius = 32.0f;
            const FVector location = puddle.Location + (puddleOrientation.GetUpVector() * puddle.Extent.Z);
            PuddleHelpers::DrawDebugRadialProgressBar(world, location, puddleOrientation,
               PuddleHelpers::GetNormalizedHealth(puddle.Health, PuddleMaxHealth),
               healthBarEmptyColor, healthBarThickness, healthBarValueColor, healthBarThickness + 3.0f, healthBarNumSegments, healthBarRadius);
         }

         if (EnumHasAnyFlags(puddleDebugFlags, PuddleHelpers::EPuddleDebugDraw::Text))
         {
            TArray<FString, TInlineAllocator<8>> parts = {
               FString::Printf(TEXT("Cluster: %s"), *GetName()),
               FString::Printf(TEXT("Puddle Id: %i"), puddle.PuddleId),
               FString::Printf(TEXT("Health: %.2f"), puddle.Health),
               FString::Printf(TEXT("Angle: %s"), *StaticEnum<ETATPuddleSurfaceAngle>()->GetNameStringByValue(static_cast<int64>(puddleState->SurfaceAngle))),
            };
            if (puddle.IsPendingRemove())
            {
               parts.Add(FString::Printf(TEXT("ServerPendingRemove: %.2f sec"), puddle.GetRemainingLifeSpan(serverWorldTimeSeconds)));
            }
            if (puddleState && puddleState->IsClientPendingRemove())
            {
               parts.Add(FString::Printf(TEXT("ClientPendingRemove: %.2f sec"), puddleState->GetClientRemainingLifeSpan(clientWorldTimeSeconds)));
            }
            DrawDebugString(world, puddle.Location, FString::Join(parts, TEXT("\n")), nullptr, FColor::Cyan, 0.0f, true);
         }
      }
   }
#endif
}

bool ATATPuddleCluster::WantsToHandleCharacterLandedEvents(ATATCharacterBase* character) const
{
   return EnableCharacterLandedOnPuddleEvents && _puddles.Num() > 0;
}

void ATATPuddleCluster::OnCharacterLandedOnThisActor(ATATCharacterBase* character, const FHitResult& landedHit, UPrimitiveComponent* overlappedComponent, bool& outInterceptLandedEvent)
{
   if (!IsValid(character))
   {
      return;
   }

   // Find the puddle that the character landed on
   int32 puddleId = INDEX_NONE;
   int32 puddleIndex = INDEX_NONE;

   // If we know the overlapping component, then just find the puddle id for that component
   if (overlappedComponent != nullptr)
   {
      for (const auto& puddle : _puddleStates)
      {
         if (overlappedComponent == puddle.Value.Collision)
         {
            puddleId = puddle.Key;
            puddleIndex = puddle.Value.PuddleIndex;
            break;
         }
      }
   }
   else
   {
      // Find a puddle that's overlapping the bottom of the character's capsule
      FVector characterLocation = character->GetActorLocation();
      if (UCapsuleComponent* capsuleComponent = character->GetCapsuleComponent())
      {
         characterLocation = capsuleComponent->GetComponentLocation() - FVector(0, 0, capsuleComponent->GetScaledCapsuleHalfHeight());
      }
      int32 closestPuddleId = INDEX_NONE;
      int32 closestPuddleIndex = INDEX_NONE;
      float closestPuddleDistanceSquared = std::numeric_limits<float>::max();
      for (int32 puddleIdx = 0; puddleIdx < _puddles.Num(); puddleIdx++)
      {
         const FTATPuddle& puddle = _puddles[puddleIdx];
         const FSphere sphere = puddle.GetBoundingSphere();
         if (sphere.IsInside(characterLocation))
         {
            const float dist = FVector::DistSquared(characterLocation, sphere.Center);
            if (dist < closestPuddleDistanceSquared)
            {
               closestPuddleId = puddle.PuddleId;
               closestPuddleIndex = puddleIdx;
               closestPuddleDistanceSquared = dist;
            }
         }
      }
      puddleId = closestPuddleId;
      puddleIndex = closestPuddleIndex;
   }

   // If the character landed on a valid puddle, fire the relevant events
   if (puddleId != INDEX_NONE && puddleIndex != INDEX_NONE && !_puddles[puddleIndex].IsPendingRemove() && _puddles[puddleIndex].Health > 0)
   {
      // If we want to prevent fall damage, intercept the character landed event to suppress default behavior (like fall damage)
      if (PreventCharacterFallDamageWhenLandingOnPuddle)
      {
         outInterceptLandedEvent = true;
      }

      OnCharacterLandedOnPuddle(puddleId, character);
   }
}

int32 ATATPuddleCluster::AuthorityAddPuddle(const FTATPuddleTransform& puddleTransform)
{
   check(HasAuthority());

   // Make sure we're not adding puddles after being destroyed
   if (!ensure(GetLifeSpan() <= 0))
   {
      return INDEX_NONE;
   }

   UWorld* world = GetWorld();
   check(world != nullptr);

   const int32 puddleId = _authorityNextPuddleId;
   check(puddleId != INDEX_NONE);
   ++_authorityNextPuddleId;

   // Create a new puddle
   FTATPuddle newPuddle{};
   newPuddle.PuddleId = puddleId;
   newPuddle.Location = puddleTransform.Location;
   newPuddle.Extent = puddleTransform.Extent;
   newPuddle.Rotation = puddleTransform.Rotation;
   newPuddle.Health = PuddleMaxHealth;

   OnPuddlePreAdded(newPuddle);

   FlushNetDormancy();
   _puddles.Add(newPuddle);
   const int32 puddleIndex = _puddles.Num() - 1;
   _puddleStates.Add(puddleId, _CreatePuddleState(newPuddle, puddleIndex));

   // No need to call _OnRep_Puddles on the server in this case - we already created the new puddle components
   _SyncPuddleStates();

   OnPuddleAdded(newPuddle);

   return puddleId;
}

bool ATATPuddleCluster::AuthorityResetPuddleHealthToMax(int32 puddleId)
{
   check(HasAuthority());

   FPuddlePair pair = _GetPuddleAndStateById(puddleId);
   if (!pair || pair.Puddle->IsPendingRemove() || pair.Puddle->Health <= 0)
   {
      return false;
   }

   // Already at full health
   if (pair.Puddle->Health >= PuddleMaxHealth)
   {
      return true;
   }

   FlushNetDormancy();
   pair.Puddle->Health = PuddleMaxHealth;
   _UpdatePuddleState(*pair.State, *pair.Puddle);
   // No need to call _OnRep_Puddles on the server in this case - we already updated the puddle components
   _SyncPuddleStates();
   return true;
}

int32 ATATPuddleCluster::AuthorityAddOrRefreshPuddle(bool& outIsNewPuddle, const FTATPuddleTransform& puddleTransform, float maxRefreshDistance, float minHealthToAllowRefresh)
{
   check(HasAuthority());

   // Make sure we're not adding puddles after being destroyed
   if (!ensure(GetLifeSpan() <= 0))
   {
      outIsNewPuddle = false;
      return INDEX_NONE;
   }

   float bestPuddleDistSquared = std::numeric_limits<float>::max();
   int32 bestPuddleId = INDEX_NONE;

   const float minPuddleDistSquared = FMath::Square(puddleTransform.GetBoundingSphere().W * 0.5f);
   const float maxPuddleDistSquared = (maxRefreshDistance > 0) ? FMath::Square(maxRefreshDistance) : std::numeric_limits<float>::max();

   minHealthToAllowRefresh = FMath::Max(0.0f, minHealthToAllowRefresh);

   for (auto pair : _puddleStates)
   {
      const int32 puddleId = pair.Key;
      const FTATPuddleState& state = pair.Value;
      if (!_puddles.IsValidIndex(pair.Value.PuddleIndex))
      {
         continue;
      }
      const FTATPuddle& puddle = _puddles[pair.Value.PuddleIndex];
      if (puddle.IsPendingRemove() || puddle.Health < minHealthToAllowRefresh)
      {
         continue;
      }

      const float puddleDistSquared = FVector::DistSquared(puddle.Location, puddleTransform.Location);
      if (puddleDistSquared > maxPuddleDistSquared)
      {
         // too far away
         continue;
      }

      // practically overlapping - just use this one
      if (puddleDistSquared < minPuddleDistSquared && AuthorityResetPuddleHealthToMax(puddleId))
      {
         outIsNewPuddle = false;
         return puddleId;
      }

      // find the closest puddle to our target 
      if (puddleDistSquared < bestPuddleDistSquared)
      {
         bestPuddleDistSquared = puddleDistSquared;
         bestPuddleId = puddleId;
      }
   }

   if (bestPuddleId != INDEX_NONE && AuthorityResetPuddleHealthToMax(bestPuddleId))
   {
      outIsNewPuddle = false;
      return bestPuddleId;
   }

   outIsNewPuddle = true;
   return AuthorityAddPuddle(puddleTransform);
}

void ATATPuddleCluster::_AuthorityRemovePuddlesInternal(TConstArrayView<int32> puddleIds)
{
   check(HasAuthority());

   FlushNetDormancy();

   const double worldTimeSeconds = GetWorld()->GetTimeSeconds();

   for (int32 puddleId : puddleIds)
   {
      FPuddlePair pair = _GetPuddleAndStateById(puddleId);
      if (!pair)
      {
         UE_LOG(LogTATPuddleCluster, Error, TEXT("Failed to remove puddle with id %i - it does not exist"), puddleId);
         continue;
      }

      // If the puddle is not already pending remove, do it now to give blueprints a chance to clean up.
      _MarkPuddlePendingRemove(*pair.State, *pair.Puddle, 0.0f);

      OnPuddleRemoved(*pair.Puddle);

      _DestroyPuddleState(*pair.State, puddleId);

      _puddles.RemoveAt(pair.State->PuddleIndex);
      _puddleStates.Remove(puddleId);
   }

   // No need to call _OnRep_Puddles on the server in this case - we already cleaned up the puddle's components
   _SyncPuddleStates();

   // Destroy this whole cluster actor if there are no remaining puddles
   if (_puddles.IsEmpty())
   {
      _puddleClusterPendingDestroy = true;
      OnPuddleClusterPendingDestroy();

      if (PuddleClusterLifetimeAfterLastPuddleRemoved > 0)
      {
         SetLifeSpan(PuddleClusterLifetimeAfterLastPuddleRemoved);
      }
      else
      {
         Destroy();
      }
   }
}

bool ATATPuddleCluster::AuthorityRemovePuddle(int32 puddleId)
{
   check(HasAuthority());
   if (!_puddleStates.Contains(puddleId))
   {
      return false;
   }
   const TArray<int32, TInlineAllocator<2>> puddleIds = { puddleId };
   _AuthorityRemovePuddlesInternal(puddleIds);
   if (!ensure(!_puddleStates.Contains(puddleId)))
   {
      return false;
   }
   return true;
}

void ATATPuddleCluster::AuthorityApplyDamageToPuddle(int32 puddleId, float damageAmount)
{
   check(HasAuthority());
   if (damageAmount == 0)
   {
      return;
   }

   FPuddlePair pair = _GetPuddleAndStateById(puddleId);
   if (!pair || pair.Puddle->IsPendingRemove())
   {
      return;
   }

   if (!pair.Puddle->CanApplyDamage(damageAmount))
   {
      return;
   }

   const double worldTimeSeconds = GetWorld()->GetTimeSeconds();

   FlushNetDormancy();
   const bool puddlePendingRemove = pair.Puddle->ApplyDamage(damageAmount, worldTimeSeconds, PuddleLifetimeAfterRemove);
   if (puddlePendingRemove)
   {
      _MarkPuddlePendingRemove(*pair.State, *pair.Puddle);
   }
   _UpdatePuddleState(*pair.State, *pair.Puddle);
   // No need to call _OnRep_Puddles on the server in this case - we already updated the puddle components
   _SyncPuddleStates();
}

void ATATPuddleCluster::OnPuddlePreAdded_Implementation(const FTATPuddle& puddle)
{
}

void ATATPuddleCluster::OnPuddleAdded_Implementation(const FTATPuddle& puddle)
{
}

void ATATPuddleCluster::OnPuddlePendingRemove_Implementation(const FTATPuddle& puddle, float puddleLifespanRemaining)
{
}

void ATATPuddleCluster::OnPuddleRemoved_Implementation(const FTATPuddle& puddle)
{
}

void ATATPuddleCluster::OnPuddleBeginOverlap_Implementation(int32 puddleId, AActor* actor)
{
}

void ATATPuddleCluster::OnPuddleEndOverlap_Implementation(int32 puddleId, AActor* actor)
{
}

void ATATPuddleCluster::OnPuddleClusterCharacterEnter_Implementation(ACharacter* character)
{
}

void ATATPuddleCluster::OnPuddleClusterCharacterLeave_Implementation(ACharacter* character)
{
}

void ATATPuddleCluster::OnPuddleClusterPendingDestroy_Implementation()
{
}

void ATATPuddleCluster::OnCharacterLandedOnPuddle_Implementation(int32 puddleId, ACharacter* character)
{
}

bool ATATPuddleCluster::GetPuddle(int32 puddleId, FTATPuddle& puddleData) const
{
   if (FPuddlePair pair = _GetPuddleAndStateById(puddleId))
   {
      puddleData = *pair.Puddle;
      return true;
   }
   puddleData = {};
   return false;
}

bool ATATPuddleCluster::GetPuddleComponents(int32 puddleId, UBoxComponent*& boxComponent, UMaterialInstanceDynamic*& decalMaterial, UDecalComponent*& decalComponent, UNiagaraComponent*& niagaraComponent) const
{
   if (FPuddlePair pair = _GetPuddleAndStateById(puddleId))
   {
      boxComponent = pair.State->Collision;
      decalMaterial = pair.State->Material;
      decalComponent = pair.State->Decal;
      niagaraComponent = pair.State->Particles;
      return true;
   }
   boxComponent = nullptr;
   decalMaterial = nullptr;
   decalComponent = nullptr;
   niagaraComponent = nullptr;
   return false;
}

FVector ATATPuddleCluster::GetPuddleClusterCentroid() const
{
   if (_puddles.IsEmpty())
   {
      return GetActorLocation();
   }
   FVector locationSum = FVector::ZeroVector;
   for (const FTATPuddle& puddle : _puddles)
   {
      locationSum += puddle.Location;
   }
   return locationSum / _puddles.Num();
}

bool ATATPuddleCluster::IsSphereOverlappingPuddleCluster(const FVector& origin, float radius) const
{
   const FSphere sphere{ origin, radius };
   for (const FTATPuddle& puddle : _puddles)
   {
      if (sphere.Intersects(puddle.GetBoundingSphere()))
      {
         return true;
      }
   }
   return false;
}

void ATATPuddleCluster::DrawPuddleClusterDevTool()
{
#if TAT_ENABLE_DEV_TOOLS
   if (ImGui::BeginTable("##puddles", 4, ImGuiTableFlags_Resizable))
   {
      ImGui::TableSetupColumn("Highlight", ImGuiTableColumnFlags_WidthFixed, 0.15f);
      ImGui::TableSetupColumn("Id", ImGuiTableColumnFlags_WidthFixed, 0.15f);
      ImGui::TableSetupColumn("Health");
      ImGui::TableSetupColumn("Overlapping Actors");

      ImGui::TableHeadersRow();

      for (int32 i = 0; i < _puddles.Num(); i++)
      {
         TATImGui::FScopedID id{ i };

         const FTATPuddle& puddle = _puddles[i];
         FTATPuddleState* state = _puddleStates.Find(puddle.PuddleId);

         if (ImGui::TableNextColumn())
         {
            if (state != nullptr)
            {
               ImGui::Checkbox("##selected", &state->SelectedInDevTool);
            }
            else
            {
               ImGui::Dummy({ 0, 0 });
            }
         }

         if (ImGui::TableNextColumn())
         {
            TATImGui::Text(TEXT("%i"), puddle.PuddleId);
         }

         if (ImGui::TableNextColumn())
         {
            bool value = puddle.IsPendingRemove();
            ImGui::BeginDisabled();
            ImGui::Checkbox("##pending-remove", &value);
            ImGui::EndDisabled();
            ImGui::SetItemTooltip("Pending removal");

            ImGui::SameLine();

            ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
            TATImGui::ProgressBar(PuddleHelpers::GetNormalizedHealth(puddle.Health, PuddleMaxHealth), FVector2f::ZeroVector, FString::Printf(TEXT("%.2f"), puddle.Health));
         }

         if (ImGui::TableNextColumn())
         {
            if (state != nullptr)
            {
               TATImGui::Text(TEXT("%i"), state->ActorsOverlappingPuddle.Num());
            }
         }
      }

      ImGui::EndTable();
   }

#if TAT_ALLOW_PUDDLE_DEBUG
   // Highlight selected puddles
   for (int32 i = 0; i < _puddles.Num(); i++)
   {
      FTATPuddleState* state = _puddleStates.Find(_puddles[i].PuddleId);
      if (state != nullptr && state->SelectedInDevTool && state->Collision)
      {
         const FTransform transform = state->Collision->GetComponentTransform();
         DrawDebugBox(GetWorld(), transform.GetLocation(), state->Collision->GetScaledBoxExtent(), transform.GetRotation(), FColor::White);
      }
   }
#endif // TAT_ALLOW_PUDDLE_DEBUG

#endif // TAT_ENABLE_DEV_TOOLS
}

void ATATPuddleCluster::_AuthorityUpdatePuddles()
{
   check(HasAuthority());

   //TODO: Toggle this update function on or off based on if anything needs to be polled

   const double worldTimeSeconds = GetWorld()->GetTimeSeconds();
   if (_authorityLastPuddleDamageUpdateWorldTime == 0)
   {
      _authorityLastPuddleDamageUpdateWorldTime = worldTimeSeconds - (1.0 / 60.0);
   }
   const float deltaSeconds = worldTimeSeconds - _authorityLastPuddleDamageUpdateWorldTime;
   _authorityLastPuddleDamageUpdateWorldTime = worldTimeSeconds;

   // Bail now if there's nothing to do
   const bool puddleStateBasedDamageEnabled = _numPuddlesTakingStateBasedDamagePerSecond > 0;
   const bool puddleMovementEffectsEnabled = PuddleActorMovementEffects.Num() > 0 && _charactersInCluster.Num() > 0;
   if (!puddleStateBasedDamageEnabled && !puddleMovementEffectsEnabled && _numPuddlesPendingRemove == 0)
   {
      return;
   }

   SCOPE_CYCLE_COUNTER(STAT_PuddleCluster_AuthorityUpdate);

   int32 numPuddleUpdates = 0;

   auto applyStateBasedPuddleDamage = [this, &numPuddleUpdates, deltaSeconds, worldTimeSeconds](FTATPuddle& puddle, FTATPuddleState& state)
   {
      // State-based damage includes weather-based damage and angle-based damage (eg. washing away in the rain, or dripping down a wall)
      float damageAmount = 0.0f;
      if (state.IsOutside)
      {
         damageAmount += _outsidePuddleDamagePerSecond;
      }
      if (PuddleHealthLossPerSecondFromAngle[static_cast<int32>(state.SurfaceAngle)] > 0.0f)
      {
         damageAmount += PuddleHealthLossPerSecondFromAngle[static_cast<int32>(state.SurfaceAngle)];
      }

      damageAmount *= deltaSeconds;

      if (!puddle.CanApplyDamage(damageAmount))
      {
         return;
      }
      const bool puddlePendingRemove = puddle.ApplyDamage(damageAmount, worldTimeSeconds, PuddleLifetimeAfterRemove);
      if (puddlePendingRemove)
      {
         _MarkPuddlePendingRemove(state, puddle);
      }
      ++numPuddleUpdates;
   };

   auto applyPuddleDamageFromMovement = [this, &numPuddleUpdates, deltaSeconds, worldTimeSeconds](FTATPuddle& puddle, FTATPuddleState& state)
   {
      for (const TWeakObjectPtr<AActor>& weakActor : state.ActorsOverlappingPuddle)
      {
         AActor* actor = weakActor.Get();
         if (actor == nullptr)
         {
            continue;
         }
         for (const FTATPuddleMovementEffect& movementEffect : PuddleActorMovementEffects)
         {
            if (!movementEffect.IsPuddleDamageEnabled() || !UTATPuddleUtilities::TargetMatchesPuddleFilter(actor, movementEffect.TargetFilter))
            {
               continue;
            }
            const float damageThisFrame = movementEffect.GetPuddleDamagePerSecond(actor->GetVelocity()) * deltaSeconds;
            if (!puddle.CanApplyDamage(damageThisFrame))
            {
               continue;
            }
            const bool puddlePendingRemove = puddle.ApplyDamage(damageThisFrame, worldTimeSeconds, PuddleLifetimeAfterRemove);
            if (puddlePendingRemove)
            {
               _MarkPuddlePendingRemove(state, puddle);
               // No reason to keep iterating over this puddle now that it's dead
               return;
            }
            ++numPuddleUpdates;
         }
      }
   };

   auto applyMovementEffects = [this, deltaSeconds]()
   {
      for (int32 effectIdx = 0; effectIdx < PuddleActorMovementEffects.Num(); effectIdx++)
      {
         const FTATPuddleMovementEffect& movementEffect = PuddleActorMovementEffects[effectIdx];
         if (!movementEffect.IsGameplayEffectEnabled())
         {
            continue;
         }

         const TOptional<float> effectDeltaSeconds = _AuthorityGetMovementGameplayEffectApplicationDeltaSeconds(effectIdx);

         const bool tryApplyEffect = !effectDeltaSeconds
            || movementEffect.GameplayEffect.ApplicationInterval <= 0
            || (effectDeltaSeconds && *effectDeltaSeconds >= movementEffect.GameplayEffect.ApplicationInterval);
         if (!tryApplyEffect)
         {
            continue;
         }

         // Cache the characters we want to apply the effect to so we can avoid _charactersInCluster changing out from under us while we're applying effects
         TArray<ACharacter*, TInlineAllocator<4>> characters;
         for (ACharacter* character : _charactersInCluster)
         {
            if (UTATPuddleUtilities::TargetMatchesPuddleFilter(character, movementEffect.TargetFilter))
            {
               characters.Add(character);
            }
         }
         for (ACharacter* character : characters)
         {
            _AuthorityTryApplyMovementGameplayEffect(character, effectDeltaSeconds.Get(deltaSeconds), &movementEffect);
         }
      }
   };

   // Apply damage to puddles
   if (puddleStateBasedDamageEnabled || puddleMovementEffectsEnabled)
   {
      FlushNetDormancy();

      for (FTATPuddle& puddle : _puddles)
      {
         if (puddle.IsPendingRemove())
         {
            continue;
         }

         FTATPuddleState& state = _puddleStates.FindChecked(puddle.PuddleId);

         if (puddleStateBasedDamageEnabled)
         {
            applyStateBasedPuddleDamage(puddle, state);

            if (!puddle.IsPendingRemove())
            {
               continue;
            }
         }

         if (puddleMovementEffectsEnabled)
         {
            applyPuddleDamageFromMovement(puddle, state);
         }
      }

      if (puddleMovementEffectsEnabled)
      {
         applyMovementEffects();
      }
   }

   if (numPuddleUpdates > 0)
   {
      _SyncPuddleStates();
   }

   // Remove any puddles that reached zero health (after their removal delay time has passed)
   if (_numPuddlesPendingRemove > 0)
   {
      TArray<int32, TInlineAllocator<16>> removedPuddleIds;
      for (const FTATPuddle& puddle : _puddles)
      {
         if (puddle.IsPendingRemove() && puddle.GetRemainingLifeSpan(worldTimeSeconds) <= 0)
         {
            removedPuddleIds.Add(puddle.PuddleId);
         }
      }
      _AuthorityRemovePuddlesInternal(removedPuddleIds);
   }
}

auto ATATPuddleCluster::_GetPuddleAndStateById(int32 puddleId) const -> FPuddlePair
{
   FPuddlePair result{};
   result.State = const_cast<FTATPuddleState*>(_puddleStates.Find(puddleId));
   if (result.State != nullptr
      && _puddles.IsValidIndex(result.State->PuddleIndex)
      && _puddles[result.State->PuddleIndex].PuddleId == puddleId)
   {
      result.Puddle = const_cast<FTATPuddle*>(&_puddles[result.State->PuddleIndex]);
   }
   return result;
}

FTATPuddleState ATATPuddleCluster::_CreatePuddleState(const FTATPuddle& puddle, int32 puddleIndex)
{
   FTATPuddleState result{};
   if (!ensure(puddleIndex != INDEX_NONE))
   {
      puddleIndex = _puddles.IndexOfByPredicate([&](const FTATPuddle& item) { return item.PuddleId == puddle.PuddleId; });
   }
   check(_puddles.IsValidIndex(puddleIndex));
   check(_puddles[puddleIndex].PuddleId == puddle.PuddleId);
   check(puddle.PuddleId != INDEX_NONE);
   result.PuddleIndex = puddleIndex;
   result.HealthCosmetic = puddle.Health;
   result.RandomValue = FMath::FRand();
   result.ClientSpawnWorldTime = GetWorld()->GetTimeSeconds();
   result.Collision = NewObject<UBoxComponent>(this, UBoxComponent::StaticClass());
   if (result.Collision)
   {
      result.Collision->SetIsReplicated(false);
      result.Collision->SetupAttachment(RootComponent);
      result.Collision->SetWorldLocation(puddle.Location);
      result.Collision->SetWorldRotation(puddle.Rotation);
      result.Collision->SetBoxExtent(puddle.Extent);
      result.Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
      result.Collision->SetCollisionProfileName(PuddleCollisionProfile.Name);
      result.Collision->SetCollisionObjectType(PuddleCollisionChannel);
      result.Collision->OnComponentBeginOverlap.AddDynamic(this, &ATATPuddleCluster::_OnPuddleComponentBeginOverlap);
      result.Collision->OnComponentEndOverlap.AddDynamic(this, &ATATPuddleCluster::_OnPuddleComponentEndOverlap);
      result.Collision->RegisterComponent();

      _componentToPuddleIdMap.Add(result.Collision, puddle.PuddleId);

      if (GetNetMode() != NM_DedicatedServer)
      {
         if (PuddleDecalMaterial != nullptr)
         {
            result.Decal = NewObject<UDecalComponent>(this, UDecalComponent::StaticClass());
            if (result.Decal)
            {
               auto fixupDecalExtent = [](const FVector& extent) -> FVector
               {
                  return { extent.Z, extent.X, extent.Y };
               };
               result.Decal->SetIsReplicated(false);
               result.Decal->SetRelativeRotation(FRotator{ -90.0, 0.0, 0.0 });
               result.Decal->SetupAttachment(result.Collision);
               result.Decal->SetDecalMaterial(PuddleDecalMaterial);
               result.Decal->SetDecalColor(PuddleColor);
               result.Decal->DecalSize = fixupDecalExtent(puddle.Extent + PuddleDecalExtentMargin);
               result.Material = result.Decal->CreateDynamicMaterialInstance();
               result.Decal->RegisterComponent();

               // Pass the world time at spawn to the material to allow for transition effects
               if (ensure(result.Material))
               {
                  if (PuddleDecalMaterial_SpawnTimeParamName != NAME_None)
                  {
                     result.Material->SetScalarParameterValue(PuddleDecalMaterial_SpawnTimeParamName, result.ClientSpawnWorldTime);
                  }
                  if (PuddleDecalMaterial_RandomValueParamName != NAME_None)
                  {
                     result.Material->SetScalarParameterValue(PuddleDecalMaterial_RandomValueParamName, result.RandomValue);
                  }
               }
            }
         }

         if (PuddleParticleSystem != nullptr)
         {
            result.Particles = NewObject<UNiagaraComponent>(this, UNiagaraComponent::StaticClass());
            if (result.Particles)
            {
               result.Particles->SetIsReplicated(false);
               result.Particles->SetRelativeRotation(PuddleParticlesRelativeRotation);
               result.Particles->SetupAttachment(result.Collision);
               result.Particles->SetAsset(PuddleParticleSystem);
               result.Particles->RegisterComponent();
               if (PuddleParticleSystem_ColorParamName != NAME_None)
               {
                  result.Particles->SetColorParameter(PuddleParticleSystem_ColorParamName, PuddleColor);
               }

               // Unlike the decal component, we need to be much more precise about positioning here
               FHitResult hitResult{};
               const FVector puddleTraceOffset = puddle.Rotation.Quaternion().GetUpVector() * (puddle.Extent.Z * 0.5f);
               if (GetWorld()->LineTraceSingleByChannel(hitResult, puddle.Location + puddleTraceOffset, puddle.Location - puddleTraceOffset, ECC_WorldStatic))
               {
                  result.Particles->SetWorldLocation(hitResult.Location);
               }
            }
         }
      }

      _UpdatePuddleState(result, puddle);
   }

   result.SurfaceAngle = UTATPuddleUtilities::GetPuddleSurfaceAngle(puddle.Rotation);

   // Do a few traces to determine if this puddle is inside or outside.
   // Because puddles are stationary, we only have to do these traces once.
   const bool debugDrawPuddleTraces = EnumHasAnyFlags(PuddleHelpers::GetPuddleDebugDraw(), PuddleHelpers::EPuddleDebugDraw::OutsideTraces);
   result.IsOutside = (IsActorInitialized() && _outsidePuddleDamagePerSecond > 0)
      ? UTATPuddleUtilities::IsPuddleOutside(this, puddle, this, debugDrawPuddleTraces)
      : false;

   // Find any actors that are already overlapping the puddle
   if (result.Collision)
   {
      TArray<AActor*> overlappingActors;
      result.Collision->GetOverlappingActors(overlappingActors);
      for (AActor* const overlappingActor : overlappingActors)
      {
         if (result.ActorsOverlappingPuddle.Contains(overlappingActor))
         {
            continue;
         }
         result.ActorsOverlappingPuddle.Add(overlappingActor);
         if (ACharacter* character = Cast<ACharacter>(overlappingActor))
         {
            _TryAddCharacterToCluster(character);
         }
      }
   }

   return result;
}

void ATATPuddleCluster::_UpdatePuddleState(FTATPuddleState& puddleState, const FTATPuddle& puddle, bool fromTick, float tickDeltaSeconds)
{
   check(puddleState.Collision);

   // Puddle is about to be removed
   if (fromTick)
   {
      if (puddleState.IsClientPendingRemove())
      {
         // This is preferred over FInterpConstantTo because it allows us to complete the fade out _just_ before the puddle is fully removed.
         puddleState.HealthCosmetic = FMath::Lerp(0.0f, puddleState.HealthCosmeticAtRemoveTime, puddleState.GetClientRemainingLifeSpanNormalized(GetWorld()->GetTimeSeconds()));
      }
      else if (!FMath::IsNearlyEqual(puddleState.HealthCosmetic, puddle.Health, 0.00001f))
      {
         puddleState.HealthCosmetic = FMath::FInterpConstantTo(puddleState.HealthCosmetic, puddle.Health, tickDeltaSeconds, PuddleHealthInterpSpeed);
      }
   }

   if (puddleState.Decal && puddleState.Material)
   {
      if (PuddleDecalMaterial_HealthParamName != NAME_None)
      {
         puddleState.Material->SetScalarParameterValue(PuddleDecalMaterial_HealthParamName, PuddleHelpers::GetNormalizedHealth(puddleState.HealthCosmetic, PuddleMaxHealth));
      }
      if (PuddleDecalMaterial_TakingWeatherDamageParamName != NAME_None)
      {
         puddleState.Material->SetScalarParameterValue(PuddleDecalMaterial_TakingWeatherDamageParamName, puddleState.IsOutside ? 1.0f : 0.0f);
      }
      if (PuddleDecalMaterial_TakingAngleDamageParamName != NAME_None)
      {
         const bool takingAngleDamage = PuddleHealthLossPerSecondFromAngle[static_cast<int32>(puddleState.SurfaceAngle)] > 0.0f;
         puddleState.Material->SetScalarParameterValue(PuddleDecalMaterial_TakingAngleDamageParamName, takingAngleDamage ? 1.0f : 0.0f);
      }
   }

   if (!fromTick && puddleState.Particles)
   {
      if (PuddleParticleSystem_HealthParamName != NAME_None)
      {
         puddleState.Particles->SetFloatParameter(PuddleParticleSystem_HealthParamName, PuddleHelpers::GetNormalizedHealth(puddle.Health, PuddleMaxHealth));
      }
   }
}

void ATATPuddleCluster::_MarkPuddlePendingRemove(FTATPuddleState& puddleState, const FTATPuddle& puddle, TOptional<float> overrideRemainingLifeSpan)
{
   UWorld* world = GetWorld();
   check(world != nullptr);

   const bool updateClientPendingRemoveTimes = GetNetMode() != NM_DedicatedServer && puddleState.ClientPendingRemoveStartWorldTime <= 0;
   if (!updateClientPendingRemoveTimes && puddleState.FiredPuddlePendingRemoveEvent)
   {
      return;
   }

   const AGameStateBase* gameState = world->GetGameState();
   if (!ensure(gameState != nullptr))
   {
      return;
   }

   const double serverWorldTimeSeconds = gameState->GetServerWorldTimeSeconds();
   const float serverPendingRemoveDuration = FMath::Max(0.0f, overrideRemainingLifeSpan ? *overrideRemainingLifeSpan : puddle.GetRemainingLifeSpan(serverWorldTimeSeconds));

   // Update any cosmetic timing values
   if (updateClientPendingRemoveTimes)
   {
      const double clientWorldTimeSeconds = world->GetTimeSeconds();

      puddleState.ClientPendingRemoveStartWorldTime = clientWorldTimeSeconds;
      puddleState.ClientPendingRemoveDuration = PuddleHelpers::GetPuddleLifeSpanCosmetic(serverPendingRemoveDuration);

      // Store the HealthCosmetic value when first set to pending removal so we can interpolate from here down to zero over the exact duration remaining.
      puddleState.HealthCosmeticAtRemoveTime = puddleState.HealthCosmetic;
   }

   // Fire the blueprint event if we haven't already
   if (!puddleState.FiredPuddlePendingRemoveEvent)
   {
      OnPuddlePendingRemove(puddle, serverPendingRemoveDuration);
      puddleState.FiredPuddlePendingRemoveEvent = true;
   }
}

void ATATPuddleCluster::_DestroyPuddleState(FTATPuddleState& puddleState, int32 puddleId)
{
   // Make a copy of the ActorsOverlappingPuddle set because it could be modified while we're iterating over it
   TArray<AActor*, TInlineAllocator<16>> actorsOverlappingPuddleCopy;
   for (const TWeakObjectPtr<AActor>& weakActor : puddleState.ActorsOverlappingPuddle)
   {
      if (AActor* actor = weakActor.Get())
      {
         actorsOverlappingPuddleCopy.Add(actor);
      }
   }

   // For any actors overlapping the puddle, treat this as an end overlap event
   for (AActor* actor : actorsOverlappingPuddleCopy)
   {
      check(actor != nullptr);

      OnPuddleEndOverlap(puddleId, actor);

      // Remove any characters in this puddle from the cluster (if they're not also in another puddle)
      ACharacter* character = Cast<ACharacter>(actor);
      if (character != nullptr && !_IsActorOverlappingAnyPuddle(character, puddleId))
      {
         _TryRemoveCharacterFromCluster(character);
      }
   }
   puddleState.ActorsOverlappingPuddle.Empty();

   // Destroy the collision component
   if (puddleState.Collision)
   {
      _componentToPuddleIdMap.Remove(puddleState.Collision);

      puddleState.Collision->OnComponentBeginOverlap.Clear();
      puddleState.Collision->OnComponentEndOverlap.Clear();
      puddleState.Collision->DestroyComponent();
      puddleState.Collision = nullptr;
   }

   // Destroy the decal component
   if (puddleState.Decal)
   {
      puddleState.Decal->DestroyComponent();
      puddleState.Decal = nullptr;
   }

   if (puddleState.Particles)
   {
      puddleState.Particles->DestroyComponent();
      puddleState.Particles = nullptr;
   }
}

void ATATPuddleCluster::_SyncPuddleStates()
{
   _numPuddlesPendingRemove = 0;
   _numPuddlesTakingStateBasedDamagePerSecond = 0;

   for (int32 puddleIndex = 0; puddleIndex < _puddles.Num(); puddleIndex++)
   {
      const FTATPuddle& puddle = _puddles[puddleIndex];
      FTATPuddleState& state = _puddleStates.FindChecked(puddle.PuddleId);
      state.PuddleIndex = puddleIndex;
      if (puddle.IsPendingRemove())
      {
         _numPuddlesPendingRemove += 1;
      }

      // NB. IsOutside is only set to true if we're configured to do weather-based damage _AND_ this puddle has been determined to be outside
      if (state.IsOutside || PuddleHealthLossPerSecondFromAngle[static_cast<int32>(state.SurfaceAngle)] > 0)
      {
         _numPuddlesTakingStateBasedDamagePerSecond += 1;
      }
   }
}

void ATATPuddleCluster::_TryAddCharacterToCluster(ACharacter* character)
{
   if (character == nullptr || _charactersInCluster.Contains(character))
   {
      return;
   }

   _charactersInCluster.Add(character);

   if (HasAuthority())
   {
      _AuthorityOnPuddleClusterEnter(character);
   }

   OnPuddleClusterCharacterEnter(character);
}

void ATATPuddleCluster::_TryRemoveCharacterFromCluster(ACharacter* character, bool force)
{
   if (character == nullptr)
   {
      return;
   }

   if (!force)
   {
      if (!_charactersInCluster.Contains(character))
      {
         return;
      }

      _charactersInCluster.Remove(character);
   }

   if (HasAuthority())
   {
      _AuthorityOnPuddleClusterLeave(character);
   }

   OnPuddleClusterCharacterLeave(character);
}

void ATATPuddleCluster::_AuthorityOnPuddleClusterEnter(ACharacter* character)
{
   check(HasAuthority());
   check(character != nullptr);

   UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(character);
   if (asc == nullptr)
   {
      return;
   }

   // Apply movement/velocity based effects first so they can read the character's initial velocity
   // before we potentially apply effects that change their max movement speed
   _AuthorityTryApplyMovementGameplayEffect(character);

   // Next, apply any gameplay effects that should be on all characters in a puddle
   for (int32 effectIdx = 0; effectIdx < GameplayEffects.Num(); effectIdx++)
   {
      const FTATPuddleGameplayEffect& effect = GameplayEffects[effectIdx];
      if (!effect.CanApplyToTarget(asc, character))
      {
         continue;
      }

      const TPair<TWeakObjectPtr<ACharacter>, int32> activeEffectKey{ character, effectIdx };

      const FActiveGameplayEffectHandle* existingHandle = _activeGameplayEffects.Find(activeEffectKey);
      if (existingHandle != nullptr && asc->GetActiveGameplayEffect(*existingHandle) != nullptr)
      {
         // we already have an effect on this character
         continue;
      }

      FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
      effectContext.AddInstigator(GetInstigator(), this);

      const FActiveGameplayEffectHandle handle = FTATGameplayEffectSetByCallerParam::ApplyGameplayEffectWithParams(
         asc, effect.GameplayEffect, effect.SetByCallerParams, effect.EffectLevel, effectContext);
      if (handle.IsValid())
      {
         _activeGameplayEffects.Add(activeEffectKey, handle);
      }
   }
}

void ATATPuddleCluster::_AuthorityOnPuddleClusterLeave(ACharacter* character)
{
   check(HasAuthority());
   check(character != nullptr);

   UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(character);
   if (asc == nullptr)
   {
      return;
   }

   for (int32 effectIdx = 0; effectIdx < GameplayEffects.Num(); effectIdx++)
   {
      const FTATPuddleGameplayEffect& effect = GameplayEffects[effectIdx];
      if (effect.AutoRemoveOnLeave && !effect.CanApplyToTarget(asc, character))
      {
         continue;
      }

      const TPair<TWeakObjectPtr<ACharacter>, int32> activeEffectKey{ character, effectIdx };

      FActiveGameplayEffectHandle* existingHandle = _activeGameplayEffects.Find(activeEffectKey);
      if (existingHandle == nullptr)
      {
         continue;
      }

      asc->RemoveActiveGameplayEffect(*existingHandle);
      _activeGameplayEffects.Remove(activeEffectKey);
   }
}

TOptional<float> ATATPuddleCluster::_AuthorityGetMovementGameplayEffectApplicationDeltaSeconds(int32 effectIdx)
{
   check(HasAuthority());

   check(PuddleActorMovementEffects.IsValidIndex(effectIdx));
   const FTATPuddleMovementEffect& movementEffect = PuddleActorMovementEffects[effectIdx];

   double* lastApplicationWorldTime = _authorityLastMovementEffectApplicationWorldTime.Find(effectIdx);
   if (lastApplicationWorldTime == nullptr)
   {
      lastApplicationWorldTime = &_authorityLastMovementEffectApplicationWorldTime.Add(effectIdx, 0.0);
   }
   check(lastApplicationWorldTime != nullptr);

   const double worldTimeSeconds = GetWorld()->GetTimeSeconds();

   TOptional<float> deltaSeconds;
   if (*lastApplicationWorldTime > 0)
   {
      deltaSeconds = worldTimeSeconds - *lastApplicationWorldTime;
   }
   *lastApplicationWorldTime = worldTimeSeconds;
   return deltaSeconds;
}

void ATATPuddleCluster::_AuthorityTryApplyMovementGameplayEffect(ACharacter* character, TOptional<float> deltaSeconds, const FTATPuddleMovementEffect* movementEffect)
{
   check(HasAuthority());
   if (character == nullptr)
   {
      return;
   }

   auto applyEffect = [character, deltaSeconds](const FTATPuddleMovementEffect& effect)
   {
      if (!effect.IsGameplayEffectEnabled())
      {
         return;
      }
      UOSEAbilitySystemComponent* asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(character);
      if (asc == nullptr || !UTATPuddleUtilities::TargetMatchesPuddleFilter(character, effect.TargetFilter))
      {
         return;
      }

      const float chanceToApply = deltaSeconds
         ? FMath::Clamp(effect.GetChancePerSecondToApplyGameplayEffect(character->GetVelocity()) * deltaSeconds.GetValue(), 0.0f, 1.0f)
         : FMath::Clamp(effect.GetChanceOnEnterToApplyGameplayEffect(character->GetVelocity()), 0.0f, 1.0f);

      if (chanceToApply > 0 && chanceToApply > FMath::FRand())
      {
         FGameplayEffectContextHandle ctx = asc->MakeEffectContext();
         asc->ApplyGameplayEffectToSelf(effect.GameplayEffect.GameplayEffect->GetDefaultObject<UGameplayEffect>(), 1.0f, ctx);
      }
   };

   // If we have an explicit movement effect, apply that, otherwise apply all of them
   if (movementEffect != nullptr)
   {
      applyEffect(*movementEffect);
   }
   else
   {
      for (const FTATPuddleMovementEffect& effect : PuddleActorMovementEffects)
      {
         applyEffect(effect);
      }
   }
}

bool ATATPuddleCluster::_IsActorOverlappingAnyPuddle(AActor* actor, int32 ignorePuddleId) const
{
   if (actor != nullptr)
   {
      for (const auto& pair : _puddleStates)
      {
         if (ignorePuddleId != INDEX_NONE && pair.Key == ignorePuddleId)
         {
            continue;
         }
         if (pair.Value.ActorsOverlappingPuddle.Contains(actor))
         {
            return true;
         }
      }
   }
   return false;
}

void ATATPuddleCluster::_OnPuddleComponentBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex,
   bool fromSweep, const FHitResult& sweepResult)
{
   const int32* puddleId = _componentToPuddleIdMap.Find(overlappedComponent);
   if (puddleId == nullptr || !ensure(*puddleId != INDEX_NONE))
   {
      return;
   }

   FTATPuddleState* puddleState = _puddleStates.Find(*puddleId);
   if (puddleState == nullptr)
   {
      return;
   }

   // Actor already overlapping this puddle (seems unlikely...)
   if (puddleState->ActorsOverlappingPuddle.Contains(otherActor))
   {
      return;
   }

   puddleState->ActorsOverlappingPuddle.Add(otherActor);

   if (ACharacter* character = Cast<ACharacter>(otherActor))
   {
      _TryAddCharacterToCluster(character);
   }

   OnPuddleBeginOverlap(*puddleId, otherActor);
}

void ATATPuddleCluster::_OnPuddleComponentEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex)
{
   const int32* puddleId = _componentToPuddleIdMap.Find(overlappedComponent);
   if (puddleId == nullptr || !ensure(*puddleId != INDEX_NONE))
   {
      return;
   }

   FTATPuddleState* puddleState = _puddleStates.Find(*puddleId);
   if (puddleState == nullptr)
   {
      return;
   }

   if (!puddleState->ActorsOverlappingPuddle.Contains(otherActor))
   {
      return;
   }

   puddleState->ActorsOverlappingPuddle.Remove(otherActor);

   // If this is a character, and they aren't in any other puddles, remove them from the whole cluster
   ACharacter* character = Cast<ACharacter>(otherActor);
   if (character != nullptr && !_IsActorOverlappingAnyPuddle(character, *puddleId))
   {
      _TryRemoveCharacterFromCluster(character);
   }

   OnPuddleEndOverlap(*puddleId, otherActor);
}

void ATATPuddleCluster::_OnRep_PuddleClusterPendingDestroy()
{
   if (_puddleClusterPendingDestroy && !HasAuthority())
   {
      OnPuddleClusterPendingDestroy();
   }
}

void ATATPuddleCluster::_OnRep_Puddles(const TArray<FTATPuddle>& oldPuddles)
{
   ensure(!HasAuthority());

   // Make a mapping of old puddle ids for easy lookups
   TMap<int32, int32, TInlineSetAllocator<16>> oldPuddleIdToIndexMap;
   for (int32 i = 0; i < oldPuddles.Num(); i++)
   {
      oldPuddleIdToIndexMap.Add(oldPuddles[i].PuddleId, i);
   }
   auto findOldPuddle = [&oldPuddleIdToIndexMap, &oldPuddles](int32 puddleId) -> const FTATPuddle*
   {
      const int32* idx = oldPuddleIdToIndexMap.Find(puddleId);
      return (idx != nullptr && oldPuddles.IsValidIndex(*idx)) ? &oldPuddles[*idx] : nullptr;
   };

   // Keep a set of current puddle ids so we can detect removed puddles
   TSet<int32, DefaultKeyFuncs<int32>, TInlineSetAllocator<16>> curPuddleIdSet;

   // Create or update puddles that currently exist
   for (int32 puddleIndex = 0; puddleIndex < _puddles.Num(); puddleIndex++)
   {
      const FTATPuddle& curPuddle = _puddles[puddleIndex];

      curPuddleIdSet.Add(curPuddle.PuddleId);

      if (FTATPuddleState* existingState = _puddleStates.Find(curPuddle.PuddleId))
      {
         // Update the index in case it's changed
         existingState->PuddleIndex = puddleIndex;

         _UpdatePuddleState(*existingState, curPuddle);

         // If this puddle just changed to pending removal, fire the callback to give blueprints a chance to play transition-out VFX/SFX
         const FTATPuddle* oldPuddle = findOldPuddle(curPuddle.PuddleId);
         if (ensure(oldPuddle != nullptr) && !oldPuddle->IsPendingRemove() && curPuddle.IsPendingRemove())
         {
            _MarkPuddlePendingRemove(*existingState, curPuddle);
         }
      }
      else
      {
         // No existing state, this is a new puddle
         OnPuddlePreAdded(curPuddle);
         _puddleStates.Add(curPuddle.PuddleId, _CreatePuddleState(curPuddle, puddleIndex));
         OnPuddleAdded(curPuddle);
      }
   }

   // Destroy and remove any puddle states that are no longer in the _puddles array
   for (const FTATPuddle& oldPuddle : oldPuddles)
   {
      const bool puddleRemoved = !curPuddleIdSet.Contains(oldPuddle.PuddleId);
      if (!puddleRemoved)
      {
         continue;
      }
      FTATPuddleState* existingState = _puddleStates.Find(oldPuddle.PuddleId);
      if (ensure(existingState != nullptr))
      {
         _MarkPuddlePendingRemove(*existingState, oldPuddle, 0.0f);
         OnPuddleRemoved(oldPuddle);
         _DestroyPuddleState(*existingState, oldPuddle.PuddleId);
         _puddleStates.Remove(oldPuddle.PuddleId);
      }
   }

   _SyncPuddleStates();
}
