// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

struct FTATLayerSceneRequirement;
struct FTATSceneVariantCollection;

// Struct that holds which layers are allowed to exist
// based on scene requirements
struct TAT_API FTATActiveLayerSet
{
public:
   FTATActiveLayerSet() = default;

   static FTATActiveLayerSet Build(TConstArrayView<FTATLayerSceneRequirement> requirements, const FTATSceneVariantCollection& variants);
   void InitFrom(TConstArrayView<FTATLayerSceneRequirement> requirements, const FTATSceneVariantCollection& variants);
   void Reset() { _layers.Reset(); }

   bool IsPopulated() const { return _layers.Num() > 0; }
   bool IsEmpty() const { return _layers.IsEmpty(); }
   
   bool IsCompatible(const AActor* actor) const;
   bool IsCompatible(TConstArrayView<FName> actorLayers) const;

   static constexpr bool kForceNetDestroy = true;
   int DestroyIncompatibleActors(UWorld* world) const;
   int DestroyIncompatibleActors(ULevel* level) const;

private:
   TMap<FName, bool> _layers;
};
