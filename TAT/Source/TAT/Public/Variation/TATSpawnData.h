// (c) 2020-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Engine/DataAsset.h"

#include "TATSpawnData.generated.h"

class UItemInfo;
class UTATSpawnerComponent;
class UTATSpawnModifier;
class UTATSceneSetAsset;
class UTATSceneAsset;
class UTATSceneVariantConfig;
struct FTATLayerSceneRequirement;
struct FTATSceneVariantSelectionParams;
struct FTATSceneVariantCollection;

///////////////////////////////////////////////////////////////////
///        FTATSpawnEntry
///////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct TAT_API FTATSpawnEntry
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
   TSoftClassPtr<AActor> ClassToSpawn;
};

///////////////////////////////////////////////////////////////////
///        UTATActorSpawnBucketAsset
///////////////////////////////////////////////////////////////////

UCLASS(BlueprintType)
class TAT_API UTATActorSpawnBucketAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bucket")
   FString DebugName;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Bucket", meta = (TitleProperty = "ClassToSpawn"))
   TArray<FTATSpawnEntry> Entries;

   TSoftClassPtr<AActor> AuthorityFindClassInBucketToSpawn(const FRandomStream& randomStream) const;
};

///////////////////////////////////////////////////////////////////
///        FTATSpawnGroup
///////////////////////////////////////////////////////////////////

USTRUCT(BlueprintType)
struct TAT_API FTATSpawnGroup
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "MapVariation.SpawnGroup"), Category = "Spawn Groups")
   FGameplayTag SpawnGroupTag;

   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawn Groups", meta = (UIMin = 0, ClampMin = 0))
   FInt32Interval SpawnCount = FInt32Interval(0, 1);

   UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Spawn Groups")
   TArray<UTATSpawnModifier*> Modifiers;

   // Requires an FMemMark in the caller (if used outside of a frame)
   // Outputs modifiers used by spawn
   void GetModifierIndicesForSpawns(TBitArray<TMemStackAllocator<>>& outModifierIndices, int numSpawns, int maxNumSpawns, const FRandomStream& randomStream) const;
};

///////////////////////////////////////////////////////////////////
///        UTATSpawnGroupAsset
///////////////////////////////////////////////////////////////////

UCLASS(BlueprintType)
class TAT_API UTATSpawnDataAsset : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (TitleProperty = "SpawnGroupTag"), Category = "Spawn Groups")
   TArray<FTATSpawnGroup> SpawnGroups;

   UPROPERTY(EditAnywhere, Category = "Scenes")
   TArray<TObjectPtr<UTATSceneSetAsset>> SceneSets;

   // Scene requirements for layers in the map to be active
   UPROPERTY(EditAnywhere, Category = "Scenes", meta=(TitleProperty="{Layer}"))
   TArray<FTATLayerSceneRequirement> LayerSceneRequirements;

#if WITH_EDITORONLY_DATA
   // Whether the map is ready to validate quest spawners
   // turn it on once all relevant spawners are in place
   // (but don't forget to)
   UPROPERTY(EditAnywhere, Category = "Validation")
   bool ValidateQuestSpawners = false;
#endif

   int32 FindGroupIndex(FGameplayTag groupTag) const;

   void SelectVariants(const FTATSceneVariantSelectionParams& params, TFunctionRef<void(const UTATSceneAsset*, const UTATSceneVariantConfig*, int32)> handler) const;
   void EmitVariantsForIndices(TConstArrayView<uint8> indices, TFunctionRef<void(const UTATSceneAsset*, const UTATSceneVariantConfig*)> handler) const;

   void ForEachVariant(TFunctionRef<void(const UTATSceneVariantConfig*)> handler) const;
   const UTATSceneAsset* FindSceneForVariant(const UTATSceneVariantConfig* variant) const;
   
   // static
   static void ValidateSpawnersSatisfyRules(const UTATSpawnDataAsset* spawnData, const TArray<UTATSpawnerComponent*>& spawners, FMessageLog& msgLog);

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};

///////////////////////////////////////////////////////////////////
///        UTATSpawnModifier
///////////////////////////////////////////////////////////////////

UENUM(BlueprintType)
enum class ETATSpawnGroupModifierRandomizationType : uint8
{
   ByPercentOfSpawnGroupSpawned,
   ByPercentOfMaxSpawnGroup,
   ByFixedMinMax,
};

UCLASS(BlueprintType, Abstract, EditInlineNew)
class TAT_API UTATSpawnModifier : public UObject
{
   GENERATED_BODY()

public:
   virtual void ApplyModifier(AActor& spawnedActor, const FRandomStream& randomStream) const { unimplemented(); }
   virtual bool ApplyBeforeActorFinishSpawn() const { return true; } // default to applying this modifier before we finish spawning

   UPROPERTY(EditAnywhere, Category = "Spawn Group Modifier")
   ETATSpawnGroupModifierRandomizationType RandomizationType = ETATSpawnGroupModifierRandomizationType::ByPercentOfSpawnGroupSpawned;
   UPROPERTY(EditAnywhere, meta = (Units = "Percent", UIMin = "0.0", UIMax = "100.0", EditCondition = "RandomizationType == ETATSpawnGroupModifierRandomizationType::ByPercentOfSpawnGroupSpawned || RandomizationType == ETATSpawnGroupModifierRandomizationType::ByPercentOfMaxSpawnGroup"), Category = "Spawn Group Modifier")
   float Percent = 100;
   UPROPERTY(EditAnywhere, meta = (EditCondition = "RandomizationType == ETATSpawnGroupModifierRandomizationType::ByFixedMinMax", EditConditionHides), Category = "Spawn Group Modifier")
   int Min = 0;
   UPROPERTY(EditAnywhere, meta = (EditCondition = "RandomizationType == ETATSpawnGroupModifierRandomizationType::ByFixedMinMax", EditConditionHides), Category = "Spawn Group Modifier")
   int Max = 1;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};

UCLASS(BlueprintType, DisplayName = "Grant Items")
class TAT_API UTATSpawnModifierGrantItems : public UTATSpawnModifier
{
   GENERATED_BODY()

public:
   virtual void ApplyModifier(AActor& spawnedActor, const FRandomStream& randomStream) const override;

   UPROPERTY(EditAnywhere, Category = "Spawn Group Modifier|Items")
   TArray<TSoftClassPtr<UItemInfo>> ItemInfos;
   
   UPROPERTY(EditAnywhere, Category = "Spawn Group Modifier|Items")
   int MinItems = 1;
   UPROPERTY(EditAnywhere, Category = "Spawn Group Modifier|Items")
   int MaxItems = 1;

#if WITH_EDITOR
   // from UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif
};

UCLASS(BlueprintType, DisplayName = "Apply Effects With Magnitude")
class TAT_API UTATSpawnModifierApplyEffectsWithMagnitude : public UTATSpawnModifier
{
   GENERATED_BODY()

public:
   virtual void ApplyModifier(AActor& spawnedActor, const FRandomStream& randomStream) const override;
   virtual bool ApplyBeforeActorFinishSpawn() const { return false; } // need to apply effects after we finish spawning

   UPROPERTY(EditAnywhere, Category = "Spawn Group Modifier|Effects")
   TArray<FOSEEffectWithSetByCallerTagAndMagnitude> Effects;
};

UCLASS(BlueprintType, DisplayName = "Apply Effects")
class TAT_API UTATSpawnModifierApplyEffects : public UTATSpawnModifier
{
   GENERATED_BODY()

public:
   virtual void ApplyModifier(AActor& spawnedActor, const FRandomStream& randomStream) const override;
   virtual bool ApplyBeforeActorFinishSpawn() const { return false; } // need to apply effects after we finish spawning

   UPROPERTY(EditAnywhere, Category = "Spawn Group Modifier|Effects")
   TArray<TSubclassOf<UGameplayEffect>> Effects;
};
