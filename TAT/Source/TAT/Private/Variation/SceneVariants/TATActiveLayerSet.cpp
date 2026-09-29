// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATActiveLayerSet.h"

// tat
#include "EngineUtils.h"
#include "Variation/SceneVariants/TATLayerSceneRequirement.h"
#include "Variation/SceneVariants/TATSceneVariantCollection.h"


FTATActiveLayerSet FTATActiveLayerSet::Build(TConstArrayView<FTATLayerSceneRequirement> requirements, const FTATSceneVariantCollection& variants)
{
   FTATActiveLayerSet result;
   result.InitFrom(requirements, variants);
   return result;
}

void FTATActiveLayerSet::InitFrom(TConstArrayView<FTATLayerSceneRequirement> requirements, const FTATSceneVariantCollection& variants)
{
   _layers.Reset();
   for (const FTATLayerSceneRequirement& requirement : requirements)
   {
      const bool isMet = variants.ResolveBool(requirement.Requirement);
      _layers.Add(requirement.Layer, isMet);
   }
}

bool FTATActiveLayerSet::IsCompatible(const AActor* actor) const
{
   return actor ? IsCompatible(actor->Layers) : true;
}

bool FTATActiveLayerSet::IsCompatible(TConstArrayView<FName> actorLayers) const
{
   if (actorLayers.IsEmpty())
   {
      return true;
   }

   // Match layer visibility rules that the editor used
   // Any positive layer allows it to exist
   // Layers for which there is no requirement has no effect
   bool hasDisabledLayer = false;
   for (const FName& layer : actorLayers)
   {
      if (const bool* layerAllowed = _layers.Find(layer))
      {
         if (*layerAllowed)
         {
            return true;
         }
         else
         {
            hasDisabledLayer = true;
         }
      }
   }

   return !hasDisabledLayer;
}

int FTATActiveLayerSet::DestroyIncompatibleActors(UWorld* world) const
{
   TRACE_CPUPROFILER_EVENT_SCOPE(FTATActiveLayerSet::DestroyIncompatibleActors(World))
   check(world);
   // Skip replication of these destructions, since clients will also do that separately
   UE::Net::FScopedIgnoreStaticActorDestruction ignoredActorDestruction;
   int count = 0;
   // It _should_ be safe to destroy in the loop given that it keeps a
   // local array, but do verify
   for (AActor* actor : TActorRange<AActor>(world))
   {
      if (!IsCompatible(actor))
      {
         actor->Destroy(kForceNetDestroy);
         ++count;
      }
   }
   return count;
}

int FTATActiveLayerSet::DestroyIncompatibleActors(ULevel* level) const
{
   TRACE_CPUPROFILER_EVENT_SCOPE(FTATActiveLayerSet::DestroyIncompatibleActors(Level))
   check(level);
   // Skip replication of these destructions, since clients will also do that separately
   UE::Net::FScopedIgnoreStaticActorDestruction ignoredActorDestruction;
   int count = 0;
   // NOTE: The logic in destroying actors appears to keep holes, rather than
   //       resizing the array. But still iterating backwards just in case.
   for (int i = level->Actors.Num() - 1; i >= 0; i--)
   {
      AActor* actor = level->Actors[i];
      if (!IsCompatible(actor))
      {
         actor->Destroy(kForceNetDestroy);
         ++count;
      }
   }
   return count;
}
