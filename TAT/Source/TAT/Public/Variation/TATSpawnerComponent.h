// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/Clues/TATClueLocationInterface.h"
#include "Variation/SceneVariants/TATSceneRequirement.h"

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Components/ActorComponent.h"

#include "TATSpawnerComponent.generated.h"

enum class ETATSpawnTiming : uint8;
class ATATSpawnerActivatedMapActor;
class UTATActorSpawnBucketAsset;
class UTATSpawnModifier;

UENUM(BlueprintType)
enum class ETATSpawnChanceType : uint8
{
   UseSpawnGroup  UMETA(ToolTip = "Spawn group rules determine our chance to spawn here"),
   Always         UMETA(ToolTip = "Always spawn 100% of the time"),
   PercentChance  UMETA(ToolTip = "Spawn based on a percent defined by 'SpawnChancePercent'"),
   Disabled       UMETA(ToolTip = "Useful to allow adding this to blueprints as an optional component and no-op the whole process"),
};

UENUM()
enum class ETATSpawnerDependencyType : uint8
{
   None,
   RequireParentSpawn UMETA(DisplayName = "Require Spawn", ToolTip = "Only spawn if the dependency spawns"),
   RequireParentNotSpawn UMETA(DisplayName = "Require Not Spawned", ToolTip = "Suppress spawning if the parent spawns")
};

UENUM()
enum class ETATSpawnBucketBehavior : uint8
{
   // May have a spawn bucket, but is not required
   Maybe,
   // Never should have a spawn bucket assigned
   Never,
   // Requires a spawn bucket to be set
   Required
};

UENUM()
enum class ETATSpawnerClueMode : uint8
{
   // This spawner will never produce clues
   Never,
   // This spawner will produce clues if the actor it spawns has clues
   FromSpawnedActor
};

// The resolved parent of a spawner component
// Can be either a spawner component or an external spawner or neither (but not both)
struct FTATSpawnerParent
{
   UTATSpawnerComponent* Spawner = nullptr;
   AActor* ExternalSpawner = nullptr;

   FTATSpawnerParent() = default;
   explicit FTATSpawnerParent(UTATSpawnerComponent* spawner)
      : Spawner(spawner)
      {}
   explicit FTATSpawnerParent(AActor* externalActor)
      : ExternalSpawner(externalActor)
   {}

   bool IsValid() const
   {
      return Spawner != nullptr || ExternalSpawner != nullptr;
   }

   FString ToString() const;
};

USTRUCT(BlueprintType)
struct TAT_API FTATVariationSpawnContext
{
   GENERATED_BODY()
public:
};

UCLASS(BlueprintType, Blueprintable, meta = (BlueprintSpawnableComponent), ClassGroup = "TAT", HideCategories = (Tags, AssetUserData, Activation, Collision, Cooking))
class TAT_API UTATSpawnerComponent : public UActorComponent, public ITATClueLocationInterface
{
   GENERATED_BODY()

public:
   UTATSpawnerComponent();

   UFUNCTION(BlueprintPure, Category = "Spawner")
   const FGameplayTag& GetSpawnGroupTag() const { return SpawnGroup; }
   UFUNCTION(BlueprintPure, Category = "Spawner")
   UTATActorSpawnBucketAsset* GetSpawnBucketAsset() const { return SpawnBucket; }
   UFUNCTION(BlueprintPure, Category = "Spawner")
   ETATSpawnChanceType GetSpawnType() const { return SpawnType; }
   UFUNCTION(BlueprintPure, Category = "Spawner")
   float GetSpawnChancePercent() const { return SpawnChancePercent; }

   ETATSpawnBucketBehavior GetSpawnBucketBehavior() const { return SpawnBucketBehavior; }
   ETATSpawnTiming GetSpawnTiming() const { return SpawnTiming; }
   
   // Returns the number of actor instances this spawner is responsible for producing
   virtual int32 GetMaxInstancesToSpawn() const;

   bool IsPrimarySpawnerForActor() const { return _primarySpawnerForActor; }
   void SetPrimarySpawnerForActor(bool primary) { _primarySpawnerForActor = primary; }

   void SetSpawnType(ETATSpawnChanceType spawnType) { SpawnType = spawnType; }
   void SetSpawnBucketBehavior(ETATSpawnBucketBehavior spawnBucketBehavior) { SpawnBucketBehavior = spawnBucketBehavior; }
   void AuthoritySpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);
   AActor* AuthoritySpawnActorDeferred(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClass, const FRandomStream& randomStream);
   void AuthorityFinishSpawnActor(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClass, const FRandomStream& randomStream, AActor* actorInstance);
   void AuthorityNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAuthorityOnSpawn, const FTATVariationSpawnContext&, spawnContext, const FRandomStream&, randomStream);
   UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly, Category = "Spawner")
   FAuthorityOnSpawn AuthorityOnSpawn;

   // Bound event should assign the spawned actor to spawnActorInstance
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FAuthorityOnSpawnActor, const FTATVariationSpawnContext&, spawnContext, const TSoftClassPtr<AActor>&, actorClass, const FRandomStream&, randomStream, AActor*&, outSpawnedActorInstance);
   UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly, Category = "Spawner")
   FAuthorityOnSpawnActor AuthorityOnSpawnActorDeferred;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_FourParams(FAuthorityOnFinishSpawnActor, const FTATVariationSpawnContext&, spawnContext, const TSoftClassPtr<AActor>&, actorClass, const FRandomStream&, randomStream, AActor*, spawnedActorInstance);
   UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly, Category = "Spawner")
   FAuthorityOnFinishSpawnActor AuthorityOnFinishSpawnActor;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAuthorityOnNotSpawn, const FTATVariationSpawnContext&, spawnContext, const FRandomStream&, randomStream);
   UPROPERTY(BlueprintAssignable, BlueprintAuthorityOnly, Category = "Spawner")
   FAuthorityOnNotSpawn AuthorityOnNotSpawn;

   ETATSpawnerDependencyType GetParentRequirementType() const { return _parentRequirement; }
   UTATSpawnerComponent* GetParentSpawner() const;
   FTATSpawnerParent GetParent() const;

   const FTATSceneRequirement& GetSceneRequirement() const { return _sceneRequirement; }

   bool IsEligibleForAutomaticOrdering() const { return SpawnType != ETATSpawnChanceType::UseSpawnGroup && SpawnType != ETATSpawnChanceType::Disabled; }

   FString GetSpawnGroupDebugString() const;
   FString GetBucketDebugString() const;

   
   // IClueLocationInterface
   virtual const FText& GetClueLocationName() const override;
   virtual const FGameplayTag& GetClueLocationTag() const override final { return _clueLocationTag; }
   // IClueLocationInterface end
   ETATSpawnerClueMode GetClueMode() const { return _clueMode; }

#if WITH_EDITOR
   void ValidateSpawnerProperties(FMessageLog& msgLog) const;
#endif

protected:
   // from UActorComponent
   virtual void InitializeComponent() override;

   AActor* _GetParentActor() const;

#if WITH_EDITOR
   virtual void OnRegister() override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void CheckForErrors() override;

   // Dependency relationships purely for editor visualization
   void _SetEditorVisSpawnerDependency(AActor* dependency);

   // Just for edit condition
   UFUNCTION()
   bool _IsTemplate() const { return IsTemplate(); }
#endif // WITH_EDITOR

private:
   // Enables map tracking for _spawnerActivatedMapActor (if _isSpawnedActorMapTracked == true)
   void _AuthorityTryEnableSpawnerActivatedMapActor();

protected:
   // How should this spawner determine it's spawn type?
   // UseSpawnGroup - Spawn group rules determine our chance to spawn here
   // Always - Always spawn 100% of the time
   // PercentChance - Spawn based on a percent defined by "SpawnChancePercent"
   // Disabled - A useful default for adding this component to actors (see: Traps) that are sometimes mission-spawn-aware and sometimes not
   UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Spawner")
   ETATSpawnChanceType SpawnType = ETATSpawnChanceType::UseSpawnGroup;

   // When we are using a weighted spawn chance, choose a value between 0-100 for a chance to spawn
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Units = "Percent", ClampMin = "0", ClampMax = "100.0", UIMin = "0.0", UIMax = "100.0", EditCondition = "SpawnType == ETATSpawnChanceType::PercentChance", EditConditionHides), Category = "Spawner")
   float SpawnChancePercent = 100.0f;

   // Which spawn group are we assigned to?  We'll use that to determine whether or not we spawn here.
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (Categories = "MapVariation.SpawnGroup", EditCondition = "SpawnType == ETATSpawnChanceType::UseSpawnGroup", EditConditionHides), Category = "Spawner")
   FGameplayTag SpawnGroup;

   // Which bucket do we spawn from?
   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (EditCondition = "SpawnType != ETATSpawnChanceType::Disabled && SpawnBucketBehavior != ETATSpawnBucketBehavior::Never", EditConditionHides), Category = "Spawner")
   UTATActorSpawnBucketAsset* SpawnBucket = nullptr;

   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Spawner")
   ETATSpawnBucketBehavior SpawnBucketBehavior = ETATSpawnBucketBehavior::Never;

   // The timing that the spawning is executed
   // 
   // All spawns are still decided up-front. This just changes when they are executed
   // Delaying a spawn may not always make sense (such as a spawner that self-destructs)
   UPROPERTY(EditAnywhere, Category = "Spawner", meta = (EditCondition = "SpawnType != ETATSpawnChanceType::Disabled", EditConditionHides))
   ETATSpawnTiming SpawnTiming;

   // Whether this is the main spawner on its owning actor
   // used for dependency and debug info
   UPROPERTY(EditDefaultsOnly, Category = "Spawner", AdvancedDisplay)
   bool _primarySpawnerForActor;

   UPROPERTY(EditInstanceOnly, Category = "Dependency", meta = (DisplayName = "Parent Requirement"))
   ETATSpawnerDependencyType _parentRequirement = ETATSpawnerDependencyType::None;

   UPROPERTY(EditInstanceOnly, meta = (DisplayName = "Parent Spawner", AllowedClasses="/Script/TAT.TATSpawnerOwnerInterface,/Script/TAT.TATExternalSpawnerDependencyInterface", EditCondition="_parentRequirement != ETATSpawnerDependencyType::None", EditConditionHides), Category = "Dependency")
   TObjectPtr<AActor> _parentSpawner = nullptr;

   UPROPERTY(EditAnywhere, Category = "Dependency", meta= (ShowOnlyInnerProperties, EditCondition = "SpawnType != ETATSpawnChanceType::Disabled || _IsTemplate()"))
   FTATSceneRequirement _sceneRequirement;

   UPROPERTY(EditDefaultsOnly, Category="Clues")
   ETATSpawnerClueMode _clueMode = ETATSpawnerClueMode::Never;

   // Tag that represents this location, so that there can be clue sets specific to it
   UPROPERTY(EditInstanceOnly, Category = "Clues", meta = (Categories = "ClueLocation", EditCondition = "_clueMode == ETATSpawnerClueMode::FromSpawnedActor", EditConditionHides))
   FGameplayTag _clueLocationTag;
   
   UPROPERTY(EditInstanceOnly, Category="Clues", meta=(EditCondition = "_clueMode != ETATSpawnerClueMode::Never", EditConditionHides))
   FText _clueLocationText;

   UPROPERTY(EditDefaultsOnly, Category = "Spawner|Map")
   bool _isSpawnedActorMapTracked = false;

   UPROPERTY(EditInstanceOnly, Category = "Spawner|Map", Meta = (EditCondition = "_isSpawnedActorMapTracked", EditConditionHides))
   TObjectPtr<ATATSpawnerActivatedMapActor> _spawnerActivatedMapActor = nullptr;
};
