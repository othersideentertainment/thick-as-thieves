// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"

#include "TATClientProxyIndicatorConfig.generated.h"

USTRUCT(BlueprintType)
struct TAT_API FTATClientProxyIndicatorConfig : public FTableRowBase
{
   GENERATED_BODY()

public:
   /// Unique gameplay tag for this indicator
   UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Indicator"))
   FGameplayTag IndicatorId;

   /// Actor class to spawn on clients to represent this indicator
   UPROPERTY(EditDefaultsOnly)
   TSoftClassPtr<AActor> IndicatorClass;

   /// This indicator should be visible to matching clients (if disabled, it will never be visible to anyone)
   UPROPERTY(EditDefaultsOnly)
   bool IsEnabled = true;

   /// If enabled, this indicator is only visible to players with the Thief Vision ability active
   UPROPERTY(EditDefaultsOnly)
   bool RequireThiefVision = false;

   /// If specified, this indicator is only visible to players that have all of these gameplay tags
   UPROPERTY(EditDefaultsOnly)
   FGameplayTagContainer RequireGameplayTags;

   /// If enabled, indicators of this type that were created by a character will be removed if that character is knocked out.
   UPROPERTY(EditDefaultsOnly)
   bool AutoRemoveOnInstigatorKnockout = false;

   /// How long the indicator will live (in seconds) before being destroyed.
   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float IndicatorLifeSpan = 30.0f;

   /// If an indicator is spawned outside in a weather type listed here, use this lifespan instead of the default one above
   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0", Categories = "Weather.Type"))
   TMap<FGameplayTag, float> IndicatorLifeSpanOutsideByWeatherType;

   /// Minimum distance this indicator can spawn to an existing indicator of the same type.
   /// You can also pass a deduplicate distance at indicator spawn time to increase this for a particular spawn.
   UPROPERTY(EditDefaultsOnly, meta = (ClampMin = "0.0", UIMin = "0.0"))
   float MinimumDeduplicateDistance = 25.0f;

   /// Can this indicator be rotated (eg. sprite glyphs always face the camera and never need to replicate their rotation to clients)
   UPROPERTY(EditDefaultsOnly)
   bool AllowRotation = false;

   /// Can this indicator be scaled (if false, the actor will always be set to a scale of [1,1,1])
   UPROPERTY(EditDefaultsOnly)
   bool AllowScale = false;

   /// Should this indicator always be visible to players regardless of distance?
   UPROPERTY(EditDefaultsOnly)
   bool VisibleAtInfiniteRange = false;

   /// The indicator will only be visible to players within this distance of it.
   /// If zero, it will instead use the value defined in TAT Project Settings -> DefaultClientProxyIndicatorMaxVisibleDistance.
   UPROPERTY(EditDefaultsOnly, meta = (EditCondition = "!VisibleAtInfiniteRange", ClampMin = "0.0", UIMin = "0.0"))
   float MaxVisibleRange = 0.0f;

};
