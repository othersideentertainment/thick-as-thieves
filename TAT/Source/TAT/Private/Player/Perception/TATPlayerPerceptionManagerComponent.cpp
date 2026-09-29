// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Player/Perception/TATPlayerPerceptionManagerComponent.h"

// tat
#include "Player/Perception/TATPlayerPerceptionSubsystem.h"
#include "Player/Perception/TATPlayerPerceivableComponent.h"

// ue5
#include "DrawDebugHelpers.h"
#include "Engine/LocalPlayer.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerPerceptionManagerComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATPlayerPerceptionManagerComponent, Log, All)

DECLARE_STATS_GROUP(TEXT("TATPlayerPerception"), STATGROUP_TATPlayerPerception, STATCAT_Advanced);

static TAutoConsoleVariable<int32> CVarTATPlayerPerceptionDebugDraw(
   TEXT("TAT.PlayerPerception.DebugDraw"),
   0,
   TEXT("Debug draw the player perception (0 = off, 1 = minimal, 2 = full)"),
   ECVF_Default
);

UTATPlayerPerceptionManagerComponent::UTATPlayerPerceptionManagerComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTATPlayerPerceptionManagerComponent::BeginPlay()
{
   Super::BeginPlay();

   if (APlayerController* owningController = Cast<APlayerController>(GetOwner()))
   {
      if (owningController->IsLocalPlayerController())
      {
         // Only bother ticking if we're the local player
         SetComponentTickEnabled(true);

         if (UTATPlayerPerceptionSubsystem* perceptionSubsystem = GetWorld()->GetSubsystem<UTATPlayerPerceptionSubsystem>())
         {
            // Find any perceivables that exist, and register them with the system
            const TArray<TWeakObjectPtr<UTATPlayerPerceivableComponent>>& perceivables = perceptionSubsystem->GetPerceivables();
            for (const TWeakObjectPtr<UTATPlayerPerceivableComponent> perceivablePtr : perceivables)
            {
               if (UTATPlayerPerceivableComponent* perceivable = perceivablePtr.Get())
               {
                  _OnPerceivableRegistered(perceivable);
               }
            }

            // Subscribe to know about any more perceivables that are created
            perceptionSubsystem->OnPerceivableRegistered.AddUObject(this, &UTATPlayerPerceptionManagerComponent::_OnPerceivableRegistered);
         }
         else
         {
            UE_LOG(LogTATPlayerPerceptionManagerComponent, Warning,
               TEXT("TATPlayerPerceptionComponent component could not find UTATPlayerPerceptionSubsystem subsystem"));
         }
      }
   }
   else
   {
      UE_LOG(LogTATPlayerPerceptionManagerComponent, Warning,
         TEXT("TATPlayerPerceptionComponent component attached to something other than the player controller"),
         *GetNameSafe(GetOwner()));
   }
}

void UTATPlayerPerceptionManagerComponent::EndPlay(EEndPlayReason::Type reason)
{
   if (UTATPlayerPerceptionSubsystem* perceptionsubsystem = GetWorld()->GetSubsystem<UTATPlayerPerceptionSubsystem>())
   {
      perceptionsubsystem->OnPerceivableRegistered.RemoveAll(this);
   }

   Super::EndPlay(reason);
}

void UTATPlayerPerceptionManagerComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   if (APlayerController* owningController = Cast<APlayerController>(GetOwner()))
   {
      if (APawn* playerPawn = owningController->GetPawn())
      {
         _TickPerceivables(deltaTime, owningController, playerPawn);
      }
   }
}

#if ENABLE_DRAW_DEBUG
static FColor PlayerPerceptionLevel_DebugColors[] = {
   FColor::Black,   // None,
   FColor::Blue,    // Perceived,
   FColor::Green,   // Aware,
   FColor::Yellow,  // Focused
   FColor::Red      // Fixated
};
static_assert(UE_ARRAY_COUNT(PlayerPerceptionLevel_DebugColors) == (int32)ETATPlayerPerceptionLevel::Count, "debug color array length mismatch");
#endif

void UTATPlayerPerceptionManagerComponent::_TickPerceivables(float deltaTime, APlayerController* playerController, APawn* playerPawn)
{
   DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Tick Perceivables"), STAT_PlayerPerceptionTick_TickPerceivables, STATGROUP_TATPlayerPerception);

   FVector eyesLocation;
   FRotator eyesRotation;
   playerPawn->GetActorEyesViewPoint(eyesLocation, eyesRotation);

   const FVector eyesDirection = eyesRotation.RotateVector(FVector::ForwardVector);

   ULocalPlayer* localPlayer = playerController->GetLocalPlayer();
   if(localPlayer == nullptr)
   {
      //Local Player can be null when using the spectator camera in the gameplay debugger
      return;
   }
   check(localPlayer->ViewportClient);
   FSceneViewProjectionData projectionData;
   bool haveProjectionData = localPlayer->GetProjectionData(localPlayer->ViewportClient->Viewport, projectionData);
   
   // NOTE: In some cases, we may have a zero-sized viewport on startup. In that case, we won't have any projection data, despite having a controller and viewport
   // In those cases we can simply skip ticking perceivables, since we will get a correctly-sized viewport on later frames and can tick normally
   if (!haveProjectionData)
   {
      return;
   }

   // N.B. we compute these ahead of time instead of calling ProjectWorldToScreen() because the latter involves some expensive lookups and computations,
   // so we want to avoid as much work as possible in the hot loop
   const FMatrix viewProjectionMatrix = projectionData.ComputeViewProjectionMatrix();
   const FIntRect constrainedViewRect = projectionData.GetConstrainedViewRect();

   for (FPerceivableAndStatus& perceivableAndStatus : _perceivablesAndStatus)
   {
      if (UTATPlayerPerceivableComponent* perceivable = perceivableAndStatus.Perceivable.Get())
      {
         FPerceivableFocusStatus focusStatus = _GetPerceivableScreenFocusArea(viewProjectionMatrix, constrainedViewRect, eyesLocation, eyesDirection, perceivable);

         float progressDelta = _GetPerceptionProgressDeltaForPerceivable(perceivableAndStatus, focusStatus);

         bool anyLOSChecksSucceed = false;
         if (progressDelta >= 0.0f)
         {
            // If we don't have LoS to the perceivable, it should regress regardless
            if (!_PerformLOSCheck(playerPawn, eyesLocation, perceivable))
            {
               if (perceivableAndStatus.PerceptionLevel == ETATPlayerPerceptionLevel::None)
               {
                  // Don't go past 0 since this is the lowest level
                  progressDelta = 0.0f;
               }
               else
               {
                  // Regress due to lack of LoS, based on the config for the current perception level
                  progressDelta = -1.0f * _GetConfigForLevel(perceivableAndStatus.PerceptionLevel).OnscreenNotVisibleRegressionSpeed;
               }
            }
         }

         perceivableAndStatus.UpdateWithProgressDelta(progressDelta * deltaTime);

#if ENABLE_DRAW_DEBUG
         const int32 debugDrawCVar = CVarTATPlayerPerceptionDebugDraw.GetValueOnGameThread();
         if (debugDrawCVar >= 1)
         {
            AActor* perceivableActor = perceivable->GetOwner();
            FVector perceivableLocation = perceivableActor->GetActorLocation();

            FColor statusColor = PlayerPerceptionLevel_DebugColors[(int32)perceivableAndStatus.PerceptionLevel];

            DrawDebugPoint(GetWorld(), perceivableLocation, 15.0f, statusColor, false, 0.0f, SDPG_Foreground);

            // N.B. location here is relative to perceivableActor
            DrawDebugString(GetWorld(), FVector(0.0f, 0.0f, 30.0f), UEnum::GetDisplayValueAsText(perceivableAndStatus.PerceptionLevel).ToString(), perceivableActor, FColor::White, 0.0f, true, 1.0f);

            // Extra, more-detailed debug info
            if (debugDrawCVar >= 2)
            {
               FString regionDebugString = FString::Printf(TEXT("Region: %s"), *UEnum::GetDisplayValueAsText(focusStatus.Region).ToString());
               FString areaDebugString = FString::Printf(TEXT("Screen Coverage: %.1f%%"), focusStatus.ScreenAreaPercentage * 100.0f);
               FString progressDebugString = FString::Printf(TEXT("Progress: %.1f%%"), perceivableAndStatus.ProgressToNextState * 100.0f);
               FString progressDeltaDebugString = FString::Printf(TEXT("Progress Delta: %.1f%%"), progressDelta * 100.0f);
               FString losCheckDebugString = FString::Printf(TEXT("LOS Check: %s"), anyLOSChecksSucceed ? TEXT("True") : TEXT("False"));
               FString maxDotProductDebugString = FString::Printf(TEXT("Max Dot Product: %.2f"), focusStatus.MaxForwardDotProduct);

               DrawDebugString(GetWorld(), FVector(0.0f, 0.0f,  45.0f), regionDebugString, perceivableActor, FColor::White, 0.0f, true, 1.0f);
               DrawDebugString(GetWorld(), FVector(0.0f, 0.0f,  60.0f), areaDebugString, perceivableActor, FColor::White, 0.0f, true, 1.0f);
               DrawDebugString(GetWorld(), FVector(0.0f, 0.0f,  75.0f), progressDebugString, perceivableActor, FColor::White, 0.0f, true, 1.0f);
               DrawDebugString(GetWorld(), FVector(0.0f, 0.0f,  90.0f), progressDeltaDebugString, perceivableActor, FColor::White, 0.0f, true, 1.0f);
               DrawDebugString(GetWorld(), FVector(0.0f, 0.0f, 105.0f), losCheckDebugString, perceivableActor, FColor::White, 0.0f, true, 1.0f);
               DrawDebugString(GetWorld(), FVector(0.0f, 0.0f, 120.0f), maxDotProductDebugString, perceivableActor, FColor::White, 0.0f, true, 1.0f);
            }
         }
#endif
      }
   }

   // Prune any stale perceivable pointers
   _perceivablesAndStatus.RemoveAll([](const FPerceivableAndStatus& perceivableAndStatus)
   {
      return !perceivableAndStatus.Perceivable.IsValid();
   });
}


bool UTATPlayerPerceptionManagerComponent::_PerformLOSCheck(AActor* playerPawn, FVector startingLocation, UTATPlayerPerceivableComponent* perceivable) const
{

   static const bool kTraceComplex = false;
   FCollisionQueryParams params(SCENE_QUERY_STAT(UTATPlayerPerceptionManagerComponent_PerformLOSCheck), kTraceComplex);
   params.bReturnPhysicalMaterial = false;
   params.bIgnoreTouches = true;

   params.AddIgnoredActor(playerPawn);
   params.AddIgnoredActor(perceivable->GetOwner());

   for (FVector visibilityCheckPoint : perceivable->VisibilityCheckPoints)
   {
      DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Perceivables LOS Trace"), STAT_PlayerPerceptionTick_LOSTrace, STATGROUP_TATPlayerPerception);

      const FVector checkLocation = perceivable->GetOwner()->ActorToWorld().TransformPosition(visibilityCheckPoint);

      const bool wasBlocked = playerPawn->GetWorld()->LineTraceTestByChannel(startingLocation, checkLocation, ECollisionChannel::ECC_Visibility, params);
      // If there's nothing in the way, then we can have LoS to the perceivable
      if (!wasBlocked)
      {
         return true;
      }
   }

   // We did not have LoS to any of the points on the perceivable
   return false;
}

static FBox2D GetScreenSpaceBox(const FMatrix& viewProjectionMatrix, const FIntRect& constrainedViewRect, const TStaticArray<FVector, 8> bounds3D)
{
   // Build 2D bounding box of actor in screen space
   FBox2D actorBox2D(EForceInit::ForceInitToZero);
   for (uint8 vertIdx = 0; vertIdx < 8; vertIdx++)
   {
      // N.B. ProjectWorldLocationToScreen() ends up being very expensive, so we pre-compute the matrix/viewport
      // and use the more stripped down function directly
      // NOTE: We now don't call PostProcessWorldToScreen(), which ProjectWorldLocationToScreen() would. This does not affect us for now,
      // but we may need to re-evaluate that at some point in the future
      FVector2D screenLocation;
      FSceneView::ProjectWorldToScreen(bounds3D[vertIdx], constrainedViewRect, viewProjectionMatrix, screenLocation);

      actorBox2D += screenLocation;
   }

   return actorBox2D;
}

UTATPlayerPerceptionManagerComponent::FPerceivableFocusStatus
UTATPlayerPerceptionManagerComponent::_GetPerceivableScreenFocusArea(const FMatrix& viewProjectionMatrix, const FIntRect& constrainedViewRect, FVector eyesLocation, FVector eyesDirection, UTATPlayerPerceivableComponent* perceivable) const
{
   DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Tick Perceivables - Get Focus Area"), STAT_PlayerPerceptionTick_GetPerceivableScreenFocusArea, STATGROUP_TATPlayerPerception);

   AActor* perceivableActor = perceivable->GetOwner();

   FVector perceivableCenter, perceivableExtents;
   perceivableActor->GetActorBounds(true, perceivableCenter, perceivableExtents);

   // TODO: This just checks if it's in front of us, could limit the angle even more based on FoV
   bool isActorOnScreen = ((perceivableCenter - eyesLocation) | eyesDirection) > 0.0f;

   float maxForwardDotProduct = 0.0f;

   TStaticArray<FVector, 8> boundsPoints;
   for (int32 i = 0; i < 8; i++)
   {
      const bool flipX = (i & 1) != 0;
      const bool flipY = (i & 2) != 0;
      const bool flipZ = (i & 4) != 0;

      boundsPoints[i] = perceivableCenter + (perceivableExtents * FVector(flipX ? -1.0f : 1.0f, flipY ? -1.0f : 1.0f, flipZ ? -1.0f : 1.0f));

      const float forwardDotProduct = (boundsPoints[i] - eyesLocation).GetSafeNormal() | eyesDirection;
      isActorOnScreen |= forwardDotProduct > 0.0f;
      maxForwardDotProduct = FMath::Max(maxForwardDotProduct, forwardDotProduct);
   }

   if (!isActorOnScreen)
   {
      return FPerceivableFocusStatus(ETATPerceivableScreenFocusRegion::Offscreen, 0.0f, 0.0f);
   }

   // Gets screenspace bounding box for actor, will be in pixel coordinates
   FBox2D screenSpaceBox = GetScreenSpaceBox(viewProjectionMatrix, constrainedViewRect, boundsPoints);

   int32 viewportWidth = constrainedViewRect.Width(), viewportHeight = constrainedViewRect.Height();

   // Get the size of the viewport in pixels so we can check how close the bounding box is to the center
   const FVector2D viewportSize = FVector2D(viewportWidth, viewportHeight);
   const FVector2D viewportCenter = viewportSize * 0.5f;

   const FBox2D fullscreenBox(FVector2D(0.0f, 0.0f), viewportSize);

   const FBox2D generalAwarenessBox(viewportCenter - viewportSize * GeneralAwarenessScreenPercentage * 0.5f,
                                    viewportCenter + viewportSize * GeneralAwarenessScreenPercentage * 0.5f);

   const FBox2D focusAreaBox(viewportCenter - viewportSize * FocusAreaScreenPercentage * 0.5f,
                             viewportCenter + viewportSize * FocusAreaScreenPercentage * 0.5f);

   const float onScreenPortionArea = fullscreenBox.Overlap(screenSpaceBox).GetArea();
   const float onScreenPortionAreaPercentage = onScreenPortionArea / fullscreenBox.GetArea();

   if (focusAreaBox.Overlap(screenSpaceBox).GetArea() > RequiredScreenOverlapForFocusRegion)
   {
      return FPerceivableFocusStatus(ETATPerceivableScreenFocusRegion::FocusArea, onScreenPortionAreaPercentage, maxForwardDotProduct);
   }
   else if (generalAwarenessBox.Overlap(screenSpaceBox).GetArea() > RequiredScreenOverlapForFocusRegion)
   {
      return FPerceivableFocusStatus(ETATPerceivableScreenFocusRegion::GeneralAwareness, onScreenPortionAreaPercentage, maxForwardDotProduct);
   }
   else if (onScreenPortionArea > RequiredScreenOverlapForFocusRegion)
   {
      return FPerceivableFocusStatus(ETATPerceivableScreenFocusRegion::PeripheralVision, onScreenPortionAreaPercentage, maxForwardDotProduct);
   }
   else
   {
      return FPerceivableFocusStatus(ETATPerceivableScreenFocusRegion::Offscreen, 0.0f, 0.0f);
   }
}

void UTATPlayerPerceptionManagerComponent::FPerceivableAndStatus::UpdateWithProgressDelta(float progressDelta)
{
   ProgressToNextState += progressDelta;

   if (ProgressToNextState >= 1.0f)
   {
      if (PerceptionLevel == ETATPlayerPerceptionLevel::Fixated)
      {
         // Allow it to saturate to 1.0 for some historesis, but do not go beyond that
         ProgressToNextState = 1.0f;
      }
      else
      {
         PerceptionLevel = (ETATPlayerPerceptionLevel)((int32)PerceptionLevel + 1);
         ProgressToNextState = 0.0f;
         if (UTATPlayerPerceivableComponent* perceivable = Perceivable.Get())
         {
            perceivable->SetCurrentPerceptionLevel(PerceptionLevel);
         }
      }
   }
   else if (ProgressToNextState <= -1.0f)
   {
      if (PerceptionLevel != ETATPlayerPerceptionLevel::None)
      {
         PerceptionLevel = (ETATPlayerPerceptionLevel)((int32)PerceptionLevel - 1);
         ProgressToNextState = 0.0f;
         if (UTATPlayerPerceivableComponent* perceivable = Perceivable.Get())
         {
            perceivable->SetCurrentPerceptionLevel(PerceptionLevel);
         }
      }
   }
}

const FTATPlayerPerceptionPerLevelConfig& UTATPlayerPerceptionManagerComponent::_GetConfigForLevel(ETATPlayerPerceptionLevel perceptionLevel) const
{
   switch (perceptionLevel)
   {
      case ETATPlayerPerceptionLevel::None: return NoneConfig;
      case ETATPlayerPerceptionLevel::Perceived: return PerceivedConfig;
      case ETATPlayerPerceptionLevel::Aware: return AwareConfig;
      case ETATPlayerPerceptionLevel::Focused: return FocusedConfig;
      case ETATPlayerPerceptionLevel::Fixated: return FixatedConfig;
      default: checkNoEntry();
   }

   return NoneConfig;
}

float UTATPlayerPerceptionManagerComponent::_GetPerceptionProgressDeltaForPerceivable(const UTATPlayerPerceptionManagerComponent::FPerceivableAndStatus& perceivableAndStatus, const UTATPlayerPerceptionManagerComponent::FPerceivableFocusStatus& focusState) const
{
   check(perceivableAndStatus.Perceivable.Get());

   FTATPlayerPerceptionPerLevelConfig configForCurrentLevel = _GetConfigForLevel(perceivableAndStatus.PerceptionLevel);

   if (focusState.Region >= configForCurrentLevel.RegionRequiredToProgress)
   {
      return configForCurrentLevel.ProgressSpeedCoefficient * focusState.ScreenAreaPercentage * focusState.MaxForwardDotProduct * perceivableAndStatus.Perceivable->ImportanceFactor;
   }
   else if (focusState.Region < configForCurrentLevel.RegionRequiredToNotRegress)
   {
      return -1.0f * configForCurrentLevel.OffscreenRegressionSpeed;
   }

   return 0.0f;
}

void UTATPlayerPerceptionManagerComponent::_OnPerceivableRegistered(UTATPlayerPerceivableComponent* perceivable)
{
   check(perceivable);
   check(!_perceivablesAndStatus.ContainsByPredicate([&](const FPerceivableAndStatus& perceivableAndStatus)
   {
      return perceivableAndStatus.Perceivable.Get() == perceivable;
   }));

   FPerceivableAndStatus perceivableAndStatus;
   perceivableAndStatus.Perceivable = perceivable;
   perceivableAndStatus.PerceptionLevel = ETATPlayerPerceptionLevel::None;
   perceivableAndStatus.ProgressToNextState = 0.0f;

   _perceivablesAndStatus.Add(perceivableAndStatus);
}




