// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"

#include "TATCharacterBeltComponent.generated.h"

USTRUCT(BlueprintType)
struct FTATBeltAttachmentSlot
{
   GENERATED_BODY()

public:
   /// If non-empty, this slot is only usable by actors specifying one of these types.
   /// If empty, this slot is only usable by actors that do not specify a type.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Belt Attachment Slot")
   FGameplayTagContainer AllowedSlotTypes;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Belt Attachment Slot")
   FName CharacterMeshSocket = NAME_None;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Belt Attachment Slot", Meta = (InlineEditConditionToggle))
   bool UseSocketRelativeTransform = false;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Belt Attachment Slot", Meta = (EditCondition = "UseSocketRelativeTransform"))
   FTransform SocketRelativeTransform;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Belt Attachment Slot", Meta = (InlineEditConditionToggle))
   bool OverrideRelativeActorLocation = false;

   /// Overrides attachment position to be a specific location relative to the center of the character capsule.
   /// This is intended only for debugging and development purposes.
   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Belt Attachment Slot", Meta = (EditCondition = "OverrideRelativeActorLocation"))
   FVector RelativeActorLocation = FVector::ZeroVector;

   inline bool IsValidForType(FGameplayTag tag) const
   {
      return AllowedSlotTypes.HasTag(tag) || (!tag.IsValid() && AllowedSlotTypes.IsEmpty());
   }
};

USTRUCT()
struct FTATBeltAttachedActorState
{
   GENERATED_BODY()

public:
   /// The actor attached to the character's belt
   UPROPERTY(Transient)
   TObjectPtr<AActor> Actor;

   /// Clears the slot, making it available for another actor
   inline void Reset()
   {
      Actor = nullptr;
   }
};

/// Component that manages actors spawned on a character's "belt"
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class UTATCharacterBeltComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATCharacterBeltComponent();

   // from UActorComponent
   virtual void OnRegister() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   /// Gets the number of actors attached to this character's belt.
   /// Note that an empty slot type will only match slots where the tag container AllowedSlotTypes is empty.
   UFUNCTION(BlueprintPure, Category = "TAT|Character Belt")
   int32 GetNumAttachedActors(FGameplayTag slotType) const;

   /// Get the index of an attached actor, if one exists.
   /// Returns false if no actors are attached.
   /// Note that an empty slot type will only match slots where the tag container AllowedSlotTypes is empty.
   UFUNCTION(BlueprintCallable, BlueprintPure = false, Category = "TAT|Character Belt", Meta = (ExpandBoolAsExecs = "ReturnValue"))
   bool FindFirstAttachedActorIndex(int32& index, FGameplayTag slotType) const;

   /// Gets an attached actor by index
   UFUNCTION(BlueprintPure, Category = "TAT|Character Belt")
   AActor* GetAttachedActor(int32 slotIndex) const;

   /// Attaches an existing actor to this character's belt
   UFUNCTION(BlueprintCallable, Category = "TAT|Character Belt")
   bool AttachActorToBelt(AActor* existingActor, int32& outSlotIndex, FGameplayTag requiredSlotType = FGameplayTag());

   /// Spawns a new actor of the given type and attaches it to this character's belt
   UFUNCTION(BlueprintCallable, Category = "TAT|Character Belt")
   AActor* SpawnAndAttachActorToBelt(TSubclassOf<AActor> actorClass, int32& outSlotIndex, FGameplayTag requiredSlotType = FGameplayTag());

   /// Spawns a new actor of the given type and attaches it to this character's belt.
   /// The actor is spawned deferred, and will run the given callback before spawning is finished.
   template<typename ActorType, typename Lambda>
   ActorType* SpawnAndAttachActorToBeltWithSetup(TSubclassOf<ActorType> actorClass, Lambda&& actorDeferredSetupCallback, int32& outSlotIndex, FGameplayTag requiredSlotType = FGameplayTag())
   {
      ActorType* newActor = nullptr;
      outSlotIndex = INDEX_NONE;
      if (_CanAttachActorToBelt(outSlotIndex, requiredSlotType))
      {
         FTransform transform;
         newActor = GetWorld()->SpawnActorDeferred<ActorType>(actorClass, transform, GetOwner(), nullptr, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
         if (newActor != nullptr)
         {
            actorDeferredSetupCallback(newActor);
            newActor->FinishSpawning(transform);
            _AttachActorToBelt(newActor, outSlotIndex);
         }
      }
      return newActor;
   }

   /// Detach an actor from this character's belt and returns it
   UFUNCTION(BlueprintCallable, Category = "TAT|Character Belt")
   AActor* DetachActorFromBelt(int32 slotIndex);

   /// Detach an actor from this character's belt, then immediately destroy it.
   UFUNCTION(BlueprintCallable, Category = "TAT|Character Belt")
   bool DetachAndDestroyActorFromBelt(int32 slotIndex);

   /// List of all possible locations that actors can be attached to
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "TAT|Character Belt")
   TArray<FTATBeltAttachmentSlot> BeltAttachmentSlots;

private:
   bool _CanAttachActorToBelt(int32& outOpenSlotIndex, FGameplayTag requiredSlotType) const;

   void _AttachActorToBelt(AActor* actor, int32 slotIndex);

   UPROPERTY(Transient)
   TArray<FTATBeltAttachedActorState> _beltSlotState;
};
