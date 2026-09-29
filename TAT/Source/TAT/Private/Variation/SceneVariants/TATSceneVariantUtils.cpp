// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/SceneVariants/TATSceneVariantUtils.h"

// tat
#include "Variation/SceneVariants/TATSceneRequirement.h"
#include "Variation/SceneVariants/TATSceneVariantConfig.h"
#include "Variation/TATMapVariationMgrComponent.h"
#include "Online/TATGameState.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSceneVariantUtils)

namespace SceneVariantUtils
{
   static const FTATSceneVariantCollection* FindSceneVariantCollection(const UWorld* world)
   {
      check(world);

      // CONSIDER: is there an argument for pushing the resolved variants to a subsystem for easier access?
      const ATATGameState* gameState = world->GetGameState<ATATGameState>();
      if (!ensure(gameState))
      {
         return nullptr;
      }

      const UTATMapVariationMgrComponent* variationMgr = gameState->GetMapVariationMgr();
      if (!ensure(variationMgr))
      {
         return nullptr;
      }

      return &variationMgr->GetActiveVariants();
   }
}

bool UTATSceneVariantUtils::IsSceneRequirementSet(const FTATSceneRequirement& requirement)
{
   return !requirement.IsNone();
}

bool UTATSceneVariantUtils::IsSceneRequirementNone(const FTATSceneRequirement& requirement)
{
   return requirement.IsNone();
}

bool UTATSceneVariantUtils::ResolveBoolRequirement(const UWorld* world, const FTATSceneRequirement& requirement)
{
   const FTATSceneVariantCollection* variants =  SceneVariantUtils::FindSceneVariantCollection(world);
   return variants && variants->ResolveBool(requirement);
}

ETATSceneResolveResult UTATSceneVariantUtils::TryResolveBoolRequirement(const UObject* worldContext, const FTATSceneRequirement& requirement, bool& outValue)
{
   if (requirement.IsNone())
   {
      outValue = false; //< should be ignored
      return ETATSceneResolveResult::NoRequirement;
   }

   UWorld* world = GEngine->GetWorldFromContextObject(worldContext, EGetWorldErrorMode::LogAndReturnNull);
   if (!world)
   {
      outValue = false;
      return ETATSceneResolveResult::Resolved; //< somewhat ambiguous
   }

   outValue = ResolveBoolRequirement(world, requirement);
   return ETATSceneResolveResult::Resolved;
}

ETATPrivateSpaceType UTATSceneVariantUtils::ResolvePrivacyForScene(const UWorld* world, const UTATSceneAsset* scene, ETATPrivateSpaceType fallback)
{
   const FTATSceneVariantCollection* variants = SceneVariantUtils::FindSceneVariantCollection(world);
   if (variants == nullptr)
   {
      return fallback;
   }

   const UTATSceneVariantConfig* variant = variants->FindVariantForScene(scene);
   return variant ? variant->GetSpacePrivacy() : fallback;
}
