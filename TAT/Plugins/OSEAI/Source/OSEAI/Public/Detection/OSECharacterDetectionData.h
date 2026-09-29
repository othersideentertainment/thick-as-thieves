// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "AI/Alertness/DetectionEnums.h"
#include "OSECharacterDetectionData.generated.h"

USTRUCT()
struct OSEAI_API FOSECharacterDetectionData
{
   GENERATED_BODY()

   FOSECharacterDetectionData() = default;

   FOSECharacterDetectionData(const AActor* actor)
      : Actor(actor)
   {
   }

   UPROPERTY()
   TWeakObjectPtr<const AActor> Actor = nullptr;

   UPROPERTY()
   float DetectionValue = 0.0f;

   UPROPERTY()
   EActorDetectionState DetectionState = EActorDetectionState::Observing;

   UPROPERTY()
   bool IsVisible = false;

   UPROPERTY(NotReplicated)
   float LocalVisibilityChangedTimestamp = static_cast<float>(INDEX_NONE);

   FORCEINLINE bool operator==(const FOSECharacterDetectionData& other) const
   {
      return Actor == other.Actor;
   }
   FORCEINLINE bool operator==(const AActor* actor) const
   {
      return Actor.Get() == actor;
   }
};
