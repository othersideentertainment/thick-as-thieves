// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "TATActorSpawner.h"

// ue4
#include "GameplayTagContainer.h"

#include "TATCharacterSpawner.generated.h"

class ATATCharacterAIBase;
class UCapsuleComponent;
class UTATNPCClueSpawnerComponent;

UCLASS(Blueprintable, BlueprintType)
class TAT_API ATATCharacterSpawner : public ATATActorSpawner
{
   GENERATED_BODY()

public:
   ATATCharacterSpawner();

   // from AActor
   virtual void BeginPlay() override;

   // from ATATActorSpawner
   virtual void AuthorityFinishSpawnActor(const FTATVariationSpawnContext& spawnContext, TSubclassOf<AActor> spawnClass, const FRandomStream& randomStream, AActor* spawnedActorInstance) override;

protected:
   // from ATATActorSpawner
   virtual void AuthoritySpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream) override;
   virtual AActor* AuthoritySpawnActorDeferred(const FTATVariationSpawnContext& spawnContext, TSubclassOf<AActor> spawnClass, const FRandomStream& randomStream) override;
   virtual void AuthorityOnActorSpawnedDeferred_Implementation(const FTATVariationSpawnContext& spawnContext, AActor* actor, const FRandomStream& randomStream) override;
   virtual void AuthorityNotSpawn(const FTATVariationSpawnContext& spawnContext, const FRandomStream& randomStream) override;

   UFUNCTION(BlueprintNativeEvent, Category = "Character Spawner")
   void AuthorityOnCharacterSpawnFinished(ACharacter* character, const FRandomStream& randomStream);
   virtual void AuthorityOnCharacterSpawnFinished_Implementation(ACharacter* character, const FRandomStream& randomStream) { }

protected:
   UPROPERTY(EditAnywhere, Category="Character Spawner")
   FGameplayTagContainer _allowedPrivateSpaceGameplayTags;
   
   UPROPERTY(EditAnywhere, Category="Character Spawner", meta = (AllowedClasses = "/Script/TAT.TATSmartObjectOwnerInterface"))
   TArray<TObjectPtr<AActor>> _livingWorldSmartObjectOverrides;
   
   // from AActor
   virtual void GetSimpleCollisionCylinder(float& collisionRadius, float& collisionHalfHeight) const override;
   
   // from ATATActorSpawner
   virtual void _ValidateCollision() override;
   virtual UShapeComponent* _GetShapeComponent() const override;

   // In the current implementation we don't take into account % chance to apply the modifiers
   // As they were designed to be used via spawn groups. Which the character spawners don't use at the moment
   // So we apply the spawn modifiers 100% of the time, regardless of the user settings.
   UPROPERTY(EditAnywhere, Instanced, BlueprintReadOnly, Category = "Actor Spawner")
   TArray<UTATSpawnModifier*> _modifiers;
private:
   UPROPERTY(Transient)
   UCapsuleComponent* _capsuleComponent = nullptr;

   UPROPERTY(EditAnywhere, Category = "NPC Clue Spawner")
   TObjectPtr<UTATNPCClueSpawnerComponent> _clueSpawnerComponent = nullptr;
};
