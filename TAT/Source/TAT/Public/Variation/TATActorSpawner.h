// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Variation/TATSpawnerFwd.h"
#include "Variation/TATSpawnerOwnerInterface.h"

// ose

// ue4
#include "GameFramework/Actor.h"

#include "TATActorSpawner.generated.h"

class UBillboardComponent;
class UTATSpawnModifier;

USTRUCT(BlueprintType)
struct TAT_API FTATActorSpawnerEditorSettings
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, Category = "Editor Settings")
   UTexture2D* GoodIcon = nullptr;

   UPROPERTY(EditDefaultsOnly, Category = "Editor Settings")
   UTexture2D* BadIcon = nullptr;

   UPROPERTY(EditDefaultsOnly, Category = "Editor Settings", meta = (AllowPreserveRatio))
   FVector GoodIconScale = FVector(0.25f, 0.25f, 0.25f);

   UPROPERTY(EditDefaultsOnly, Category = "Editor Settings", meta = (AllowPreserveRatio))
   FVector BadIconScale = FVector(0.25f, 0.25f, 0.25f);

   UPROPERTY(EditDefaultsOnly, Category = "Editor Settings")
   FColor ForwardDirectionArrowColor = FColor::Red;
};

UCLASS(Blueprintable, BlueprintType, HideCategories = (Lighting, LightColor, Force, Collision, Rendering, Replication, Input, LOD, HLOD, Physics, Cooking, Actor, "Actor Tick"))
class TAT_API ATATActorSpawner : public AActor, public ITATSpawnerOwnerInterface
{
   GENERATED_BODY()

public:
   ATATActorSpawner();

   // executed via callback from the mission system to spawn in this spawner, subclasses can override
   virtual void AuthoritySpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);
   virtual AActor* AuthoritySpawnActorDeferred(const FTATVariationSpawnContext& spawnContext, TSubclassOf<AActor> spawnClass, const FRandomStream& randomStream);
   virtual void AuthorityFinishSpawnActor(const FTATVariationSpawnContext& spawnContext, TSubclassOf<AActor> spawnClass, const FRandomStream& randomStream, AActor* spawnedActorInstance);
   virtual void AuthorityNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);

   UFUNCTION(BlueprintPure, Category = "Actor Spawner")
   UTATSpawnerComponent* GetSpawnerComponent() const { return _spawnerComponent; }

   bool HasSpawnedActor() const { return _spawnedActor != nullptr; }
   AActor* GetSpawnedActor() const { return _spawnedActor; }

protected:
#if WITH_EDITOR
   // from UObject
   virtual void CheckForErrors() override;
#endif

   // from AActor
   virtual void PostInitializeComponents() override;
   virtual void BeginPlay() override;
   virtual void OnConstruction(const FTransform& transform);
#if WITH_EDITOR
   virtual void PostEditMove(bool finished) override;
   virtual void PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent) override;
   virtual void PostEditUndo() override;
#endif // WITH_EDITOR

   // for our subclasses:

   // Check to make sure the navigation is at a valid point.
   // Detect when path building is going to move a pathnode around
   // This may be undesirable for LDs (ie b/c cover links define slots by offsets)
   virtual void _ValidateCollision() { };
   virtual UShapeComponent* _GetShapeComponent() const { return nullptr; }

protected:
   UPROPERTY(EditDefaultsOnly, Category = "Actor Spawner|Editor Settings")
   FTATActorSpawnerEditorSettings EditorSettings;

protected:
   void _TryValidateCollision();
   void _ApplyEditorSettings();
   bool _ShouldBeBased() const;
   APhysicsVolume* _GetNavPhysicsVolume() const;
   const FTransform& _GetSpawnTransform() const;

   UFUNCTION()
   void _AuthorityOnSpawnerSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);
   UFUNCTION()
   void _AuthorityOnSpawnerSpawnActorDeferred(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClass, const FRandomStream& randomStream, AActor*& outSpawnedActorInstance);
   UFUNCTION()
   void _AuthorityOnSpawnerFinishSpawn(const FTATVariationSpawnContext& spawnContext, const TSoftClassPtr<AActor>& actorClass, const FRandomStream& randomStream, AActor* spawnedActorInstance);
   UFUNCTION()
   void _AuthorityOnSpawnerNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);

   UFUNCTION(BlueprintNativeEvent, Category = "Actor Spawner")
   void AuthorityOnSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);
   virtual void AuthorityOnSpawn_Implementation(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream) { }

   UFUNCTION(BlueprintNativeEvent, Category = "Actor Spawner")
   void AuthorityOnActorSpawnedDeferred(const FTATVariationSpawnContext& spawnContext, AActor* actor, const FRandomStream& randomStream);
   virtual void AuthorityOnActorSpawnedDeferred_Implementation(const FTATVariationSpawnContext& spawnContext, AActor* actor, const FRandomStream& randomStream) { }

   UFUNCTION(BlueprintNativeEvent, Category = "Actor Spawner")
   void AuthorityOnActorSpawnFinished(const FTATVariationSpawnContext& spawnContext, AActor* actor, const FRandomStream& randomStream);
   virtual void AuthorityOnActorSpawnFinished_Implementation(const FTATVariationSpawnContext& spawnContext, AActor* actor, const FRandomStream& randomStream) { }

   UFUNCTION(BlueprintNativeEvent, Category = "Actor Spawner")
   void AuthorityOnNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream);
   virtual void AuthorityOnNotSpawn_Implementation(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream) { }

protected:
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Actor Spawner")
   UTATSpawnerComponent* _spawnerComponent = nullptr;

   // Arrow component to indicate forward direction of spawn
#if WITH_EDITORONLY_DATA
   UPROPERTY(Transient)
   class UArrowComponent* _arrowComponent = nullptr;
#endif

   // Normal editor sprite.
   UPROPERTY(Transient)
   UBillboardComponent* _goodSprite = nullptr;

   // Used to draw bad collision intersection in editor.
   UPROPERTY(Transient)
   UBillboardComponent* _badSprite = nullptr;

#if WITH_EDITORONLY_DATA
   // Used for validation
   TArray<ETATSpawnChanceType> _validSpawnChanceTypes;
#endif

private:
   UPROPERTY(Transient)
   AActor* _spawnedActor = nullptr;
};

DECLARE_LOG_CATEGORY_EXTERN(LogTATSpawner, Log, All);
