// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Variation/TATSpawnData.h"

// tat
#include "Items/TATItemInventoryComponent.h"
#include "Variation/MapVariationValidationUtl.h"
#include "Variation/TATSpawnerComponent.h"
#include "Variation/TATSpawnerHelper.h"
#include "Variation/TATSpawnPlan.h"
#include "Variation/SceneVariants/TATLayerSceneRequirement.h"
#include "Variation/SceneVariants/TATSceneAsset.h"
#include "Variation/SceneVariants/TATSceneSelection.h"
#include "Variation/SceneVariants/TATSceneSet.h"
#include "Variation/SceneVariants/TATSceneVariantCollection.h"

// ose
#include "Items/ItemInfo.h"

// ue4
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Logging/MessageLog.h"
#include "Misc/DataValidation.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSpawnData)

TSoftClassPtr<AActor> UTATActorSpawnBucketAsset::AuthorityFindClassInBucketToSpawn(const FRandomStream& randomStream) const
{
   if (Entries.Num() > 0)
   {
      const int indexToSpawnInto = randomStream.RandRange(0, Entries.Num() - 1);
      return Entries[indexToSpawnInto].ClassToSpawn;
   }
   return nullptr;
}

int32 UTATSpawnDataAsset::FindGroupIndex(FGameplayTag groupTag) const
{
   return SpawnGroups.IndexOfByPredicate([groupTag](const FTATSpawnGroup& group) { return group.SpawnGroupTag == groupTag;});
}

void UTATSpawnDataAsset::SelectVariants(const FTATSceneVariantSelectionParams& params, TFunctionRef<void(const UTATSceneAsset*, const UTATSceneVariantConfig*, int32)> handler) const
{
   TATSceneSelection::SelectVariants(SceneSets, params, handler);
}

void UTATSpawnDataAsset::EmitVariantsForIndices(TConstArrayView<uint8> indices, TFunctionRef<void(const UTATSceneAsset*, const UTATSceneVariantConfig*)> handler) const
{
   for (TObjectPtr<UTATSceneSetAsset> sceneSet : SceneSets)
   {
      check(sceneSet);

      const int32 numUsed = sceneSet->EmitVariantsForIndices(indices, handler);
      indices.RightChopInline(numUsed);
   }
}

void UTATSpawnDataAsset::ForEachVariant(TFunctionRef<void(const UTATSceneVariantConfig*)> handler) const
{
   for (TObjectPtr<UTATSceneSetAsset> sceneSet : SceneSets)
   {
      if(sceneSet)
      {
         sceneSet->ForEachVariant(handler);
      }
   }
}

const UTATSceneAsset* UTATSpawnDataAsset::FindSceneForVariant(const UTATSceneVariantConfig* variant) const
{
   if(variant == nullptr)
   {
      return nullptr;
   }

   for (TObjectPtr<UTATSceneSetAsset> sceneSet : SceneSets)
   {
      if(sceneSet)
      {
         if(const UTATSceneAsset* scene = sceneSet->FindSceneForVariant(variant))
         {
            return scene;
         }
      }
   }

   return nullptr;
}

/* static */
void UTATSpawnDataAsset::ValidateSpawnersSatisfyRules(const UTATSpawnDataAsset* spawnData, const TArray<UTATSpawnerComponent*>& spawners, FMessageLog& msgLog)
{
   SpawnHelpers::ValidateSpawners(spawnData->SpawnGroups, spawners, msgLog);
}

#if WITH_EDITOR
EDataValidationResult UTATSpawnDataAsset::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   for (const FTATSpawnGroup& spawnGroup : SpawnGroups)
   {
      VALIDATE_MISSION_GAMEPLAYTAG(spawnGroup.SpawnGroupTag);

      // feed validation through to the spawn group modifiers, it looks like instanced object properties don't receive the callback via the engine :(
      for(UTATSpawnModifier* modifier : spawnGroup.Modifiers)
      {
         if (modifier)
         {
            modifier->IsDataValid(context);
         }
         else
         {
            VALIDATE_ADDERROR(FString::Printf(TEXT("Empty modifier in spawn group %s"), *spawnGroup.SpawnGroupTag.ToString()));
         }
      }
   }

   VALIDATE_MISSION_ARRAY_NO_NULL_ENTRIES(SceneSets);
   UTATSceneSetAsset::ValidateDuplicates(SceneSets, this, context);

   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif // WITH_EDITOR

///////////////////////////////////////////////////////////////////
///        UTATMissionSpawnGroupModifier
///////////////////////////////////////////////////////////////////

#if WITH_EDITOR
EDataValidationResult UTATSpawnModifier::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);

   if (Min > Max)
   {
      VALIDATE_ADDERROR(FString::Printf(TEXT("Modifier %s Min must be <= Max"), *GetName()));
   }
   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif // WITH_EDITOR

void UTATSpawnModifierGrantItems::ApplyModifier(AActor& spawnedActor, const FRandomStream& randomStream) const
{
   UTATItemInventoryComponent* actorInventory = UTATItemInventoryComponent::GetTATItemInventoryFromActor(&spawnedActor);
   if (!actorInventory)
   {
      VALIDATE_MAPVARIATION_ERROR(TEXT("Tried to apply modifier %s to actor %s but it had no inventory!"), *GetName(), *spawnedActor.GetName());
      return;
   }

   int numPossibleEntries = ItemInfos.Num();
   if (numPossibleEntries > 0)
   {
      int randEntryIdx = randomStream.RandHelper(numPossibleEntries);

      // TODO: Item info class preload somewhere someday?
      if (UClass* itemInfoClass = ItemInfos[randEntryIdx].LoadSynchronous())
      {
         int numItems = randomStream.RandRange(MinItems, MaxItems);
         actorInventory->AuthorityAddItemMultiple(itemInfoClass, numItems);
      }
   }
}

#if WITH_EDITOR
EDataValidationResult UTATSpawnModifierGrantItems::IsDataValid(FDataValidationContext& context) const
{
   Super::IsDataValid(context);
   VALIDATE_MISSION_ARRAY_SOFTCLASSPTR_NO_NULL_ENTRIES(ItemInfos);
   if (ItemInfos.Num() == 0)
   {
      VALIDATE_ADDERROR(FString::Printf(TEXT("Modifier %s has no item entries!"), *GetName()));
   }
   if (MinItems > MaxItems)
   {
      VALIDATE_ADDERROR(FString::Printf(TEXT("Modifier %s MinItems must be <= MaxItems"), *GetName()));
   }
   if (Percent < 0.0f || Percent > 100.0f)
   {
      VALIDATE_ADDERROR(FString::Printf(TEXT("Modifier %s Percent must be between 0-100"), *GetName()));
   }
   return context.GetIssues().Num() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif // WITH_EDITOR

void UTATSpawnModifierApplyEffectsWithMagnitude::ApplyModifier(AActor& spawnedActor, const FRandomStream& randomStream) const
{
   for(const FOSEEffectWithSetByCallerTagAndMagnitude& effect : Effects)
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(&spawnedActor))
      {
         effect.ApplyEffect(asc);
      }
   }
}

void UTATSpawnModifierApplyEffects::ApplyModifier(AActor& spawnedActor, const FRandomStream& randomStream) const
{
   for (const TSubclassOf<UGameplayEffect>& effectClass : Effects)
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(&spawnedActor))
      {
         FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
         if (effectContext.IsValid())
         {
            FGameplayEffectSpecHandle specHandle = asc->MakeOutgoingSpec(effectClass, UGameplayEffect::INVALID_LEVEL, effectContext);
            if (specHandle.IsValid())
            {
               asc->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
            }
         }
      }
   }
}

void FTATSpawnGroup::GetModifierIndicesForSpawns(TBitArray<TMemStackAllocator<>>& outModifierIndices, int numSpawns, int maxNumSpawns, const FRandomStream& randomStream) const
{
   const int32 modifierCount = Modifiers.Num();
   outModifierIndices.Init(false, numSpawns * modifierCount);

   for (int modifierIndex = 0; modifierIndex < modifierCount; ++modifierIndex)
   {
      const UTATSpawnModifier* modifier = Modifiers[modifierIndex];

      if (!modifier)
      {
         continue;
      }

      int indicesToAdd = 0;
      float pctNorm = modifier->Percent / 100.0f;
      switch (modifier->RandomizationType)
      {
      case ETATSpawnGroupModifierRandomizationType::ByPercentOfSpawnGroupSpawned:
      {
         indicesToAdd = FMath::RoundToInt(pctNorm * float(numSpawns));
      }
      break;
      case ETATSpawnGroupModifierRandomizationType::ByPercentOfMaxSpawnGroup:
      {
         indicesToAdd = FMath::RoundToInt(pctNorm * float(maxNumSpawns));
      }
      break;
      case ETATSpawnGroupModifierRandomizationType::ByFixedMinMax:
      {
         indicesToAdd = randomStream.RandRange(modifier->Min, modifier->Max);
      }
      break;
      }

      // cap it at the max # of spawns we've made
      indicesToAdd = FMath::Min(indicesToAdd, numSpawns);
      if (indicesToAdd == 0)
      {
         continue;
      }

      FMemMark mark(FMemStack::Get());

      // generate a list of all indices we can pick from
      TArray<int, TMemStackAllocator<>> allSpawnIndices;
      allSpawnIndices.Reserve(numSpawns);
      for (int spawnIdx = 0; spawnIdx < numSpawns; ++spawnIdx)
      {
         allSpawnIndices.Add(spawnIdx);
      }

      while (indicesToAdd > 0)
      {
         const int32 indexToRemove = randomStream.RandHelper(allSpawnIndices.Num());
         const int32 randomSpawnIdx = allSpawnIndices[indexToRemove];
         allSpawnIndices.RemoveAtSwap(indexToRemove);
         outModifierIndices[(randomSpawnIdx * modifierCount) + modifierIndex] = true;
         --indicesToAdd;
      }
   }
}
