// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Indicators/TATPersistentGlyphComponent.h"

// tat
#include "Indicators/TATPersistentGlyphSubsystem.h"

// ose
#include "OSECoreCheats.h"

// ue
#include "Materials/MaterialInstanceDynamic.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPersistentGlyphComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATGlyphIndicatorComponent, Log, All);

UTATPersistentGlyphComponent::UTATPersistentGlyphComponent(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
#if OSE_CHEATS_ENABLED
   // We'll handle calling _DrawDebugGlyph ourselves
   _enableAutoDrawDebugGlyphTick = false;
#endif
}

void UTATPersistentGlyphComponent::BeginPlay()
{
   Super::BeginPlay();

   // Start invisible, until toggled on by player in range
   SetVisibility(false);

   if (AutoRegisterWithSubsystem)
   {
      RegisterIndicator();
   }
}

void UTATPersistentGlyphComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   UnregisterIndicator();

   Super::EndPlay(endPlayReason);
}

void UTATPersistentGlyphComponent::RegisterIndicator()
{
   if (UTATPersistentGlyphSubsystem* persistentGlyphSubsystem = GetWorld()->GetSubsystem<UTATPersistentGlyphSubsystem>())
   {
      persistentGlyphSubsystem->RegisterIndicator(this);
   }
}

void UTATPersistentGlyphComponent::UnregisterIndicator()
{
   if (UTATPersistentGlyphSubsystem* persistentGlyphSubsystem = GetWorld()->GetSubsystem<UTATPersistentGlyphSubsystem>())
   {
      persistentGlyphSubsystem->UnregisterIndicator(this);

      constexpr bool instant = true;
      _SetGlyphVisible(false, instant);
   }
}

void UTATPersistentGlyphComponent::RefreshVisibilityForLocalPlayer(const FVector& playerLocation)
{
#if OSE_CHEATS_ENABLED
   _DrawDebugGlyph(playerLocation);
#endif

   if (!_enableRangeBasedVisibility)
   {
      return;
   }

   const FVector playerToIndicator = GetOwner()->GetActorLocation() - playerLocation;

   // Evaluate required distance to affect visibility change, incorporating hysteresis to avoid state-flickering at the boundaries
   float requiredDistance = _visibleRange;
   if (IsGlyphVisible())
   {
      requiredDistance += _visibleRangeHysteresis;
   }

   const bool shouldBeVisible = _visibleAtInfiniteRange || playerToIndicator.SquaredLength() <= (requiredDistance * requiredDistance);
   if (IsGlyphVisible() != shouldBeVisible)
   {
      _SetGlyphVisible(shouldBeVisible);
   }
}

void UTATPersistentGlyphComponent::SetGlyphVisibility(bool newVisible)
{
   _enableRangeBasedVisibility = false;

   if (newVisible != IsGlyphVisible())
   {
      _SetGlyphVisible(newVisible);
   }
}

void UTATPersistentGlyphComponent::SetAutomaticGlyphIndicatorVisibility()
{
   _enableRangeBasedVisibility = true;
}
