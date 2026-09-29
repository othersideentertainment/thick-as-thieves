// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/TATSpawnerComponent.h"

// ue
#include "Math/Transform.h"
#include "UObject/SoftObjectPtr.h"

#include "TATContainerSpawnerComponent.generated.h"

// A slot into which an actor can be spawned by a UTATContainerSpawnerComponent
USTRUCT()
struct TAT_API FTATContainerSpawnSlotEntry
{
   GENERATED_BODY()

   // Offset from the actor's origin at which the actor should spawn
   UPROPERTY(EditDefaultsOnly, Category = "Transform", Meta = (Delta = "1.0f"))
   FVector SpawnOffset = FVector::ZeroVector;

   UPROPERTY(EditDefaultsOnly, Category = "Transform", Meta = (Delta = "5.0f"))
   FRotator SpawnRotation = FRotator::ZeroRotator;

   // Identifies this spawn slot as having been used to spawn an actor instance, so it can be skipped when selecting an unused slot
   UPROPERTY(Transient)
   TWeakObjectPtr<AActor> SpawnedActorInstance = nullptr;
};

// Component that allows for spawning actors into a collection of slots.
UCLASS(BlueprintType, ClassGroup = "TAT", HideCategories = (Tags, AssetUserData, Activation, Collision, Cooking), meta = (BlueprintSpawnableComponent))
class TAT_API UTATContainerSpawnerComponent : public UTATSpawnerComponent
{
   GENERATED_BODY()

   UTATContainerSpawnerComponent();
   
   // from UActorComponent
   virtual void InitializeComponent() override;

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
   virtual void CheckForErrors() override;
#endif // WITH_EDITOR

   // From UTATSpawnerComponent
   virtual int32 GetMaxInstancesToSpawn() const override;

public:
   // Returns the transform that should be used for spawning into the given spawnSlotEntry
   FTransform GetSpawnTransformWorldSpace(const FTATContainerSpawnSlotEntry& spawnSlotEntry) const;

private:
   UFUNCTION()
   void _AuthorityOnSpawnActorDeferred(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClassSoftPtr, const FRandomStream& randomStream, AActor*& spawnedActorInstance);
   UFUNCTION()
   void _AuthorityOnFinishSpawnActor(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClassSoftPtr, const FRandomStream& randomStream, AActor* actorInstance);

   FTATContainerSpawnSlotEntry* _FindUnusedSpawnSlot(const FRandomStream& randomStream);

   // Returns a spawn slot reserved for spawning an actor of this class type, whose actor hasn't finished spawning
   const FTATContainerSpawnSlotEntry* _FindDeferredSpawnSlot(AActor* Actor) const;

   const TArray<int32>& _GetOrPopulateEnabledSpawnSlotIndices(const FRandomStream& randomStream);

#if WITH_EDITOR
   TArray<FString> _ValidateSpawnSlots() const;
#endif // WITH_EDITOR

public:

   // Slots into which actors can be spawned
   UPROPERTY(EditAnywhere, Category = "TAT|Spawn")
   TArray<FTATContainerSpawnSlotEntry> SpawnSlotEntries;

   // When set to true, NumEnabledSlots will control how many slots are selected for spawning
   UPROPERTY(EditInstanceOnly, Category = "TAT|Spawn")
   bool LimitSlotsUsedForSpawning = false;

   // Controls number of slots that should be selected from spawning an actor
   UPROPERTY(EditAnywhere, Category = "TAT|Spawn", meta = (EditCondition = "LimitSlotsUsedForSpawning", UIMin = "1", ClampMin = 1))
   int32 NumEnabledSlots = 1;

private:
   // Indices of slots viable for spawning, after excluding disabled entries
   TArray<int32> _enabledSpawnSlotIndices;
};
