// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Character/TATCharacterBeltComponent.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Loot/TATLootTags.h"

// ue
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacterBeltComponent)

namespace BeltHelpers
{
   /// Set up placeholder defaults for belt slots, picking locations in a circle around the character's capsule,
   /// adding the "Loot.Major" tag to one of the slots.
   /// Later on this can be configured by art and design in character blueprints, but for now it's handy for testing.
   TArray<FTATBeltAttachmentSlot> MakePlaceholderBeltAttachmentSlots(int32 slotCount, float playerCapsuleRadius, int32 numMajorLootSlots)
   {
      TArray<FTATBeltAttachmentSlot> slots;

      check(numMajorLootSlots <= slotCount);

      auto getAttachAngleDegrees = [](int32 idx, int32 numSlots) -> float
      {
         // starting angle to place the first item at
         // (0 is the pawn's forward vector and 90 is the pawn's right vector)
         constexpr float startAngleDegrees = 90.0f + 45.0f;

         // total angle (slice of the pie) we'll divide up for use by slots
         constexpr float areaArcAngleDegrees = 90.0f;

         const float angleIncrement = areaArcAngleDegrees / (float)(numSlots - 1);
         return startAngleDegrees + (angleIncrement * (float)idx);
      };

      auto positionOnCircle = [](float angleDegrees, float radius)
      {
         return FVector(radius * FMath::Cos(FMath::DegreesToRadians(angleDegrees)), radius * FMath::Sin(FMath::DegreesToRadians(angleDegrees)), 0.0f);
      };

      slots.SetNum(slotCount);
      for (int32 i = 0; i < slotCount; i++)
      {
         FTATBeltAttachmentSlot& slot = slots[i];
         if (i < numMajorLootSlots)
         {
            slot.AllowedSlotTypes.AddTag(TAG_Loot_Major);
         }
         slot.CharacterMeshSocket = NAME_None;
         slot.UseSocketRelativeTransform = false;
         slot.SocketRelativeTransform = FTransform::Identity;
         slot.OverrideRelativeActorLocation = true;
         slot.RelativeActorLocation = positionOnCircle(getAttachAngleDegrees(i, slotCount), playerCapsuleRadius);
      }

      return slots;
   }
}

UTATCharacterBeltComponent::UTATCharacterBeltComponent()
   : BeltAttachmentSlots(BeltHelpers::MakePlaceholderBeltAttachmentSlots(4, 40.0f, 4))
{
}

void UTATCharacterBeltComponent::OnRegister()
{
   _beltSlotState.SetNum(BeltAttachmentSlots.Num());
   Super::OnRegister();
}

#if WITH_EDITOR
EDataValidationResult UTATCharacterBeltComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult parentResult = Super::IsDataValid(context);
   return (context.GetNumErrors() + context.GetNumWarnings() > 0) ? EDataValidationResult::Invalid : parentResult;
}
#endif

void UTATCharacterBeltComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   // destroy any attached actors just in case
   for (FTATBeltAttachedActorState& state : _beltSlotState)
   {
      if (IsValid(state.Actor))
      {
         state.Actor->Destroy();
         state.Reset();
      }
   }

   Super::EndPlay(endPlayReason);
}

int32 UTATCharacterBeltComponent::GetNumAttachedActors(FGameplayTag slotType) const
{
   check(_beltSlotState.Num() == BeltAttachmentSlots.Num());
   int32 count = 0;
   for (int32 i = 0; i < _beltSlotState.Num(); i++)
   {
      if (_beltSlotState[i].Actor != nullptr && (!slotType.IsValid() || BeltAttachmentSlots[i].IsValidForType(slotType)))
      {
         ++count;
      }
   }
   return count;
}

bool UTATCharacterBeltComponent::FindFirstAttachedActorIndex(int32& index, FGameplayTag slotType) const
{
   check(_beltSlotState.Num() == BeltAttachmentSlots.Num());
   for (int32 i = 0; i < _beltSlotState.Num(); i++)
   {
      if (_beltSlotState[i].Actor != nullptr && BeltAttachmentSlots[i].IsValidForType(slotType))
      {
         index = i;
         return true;
      }
   }
   index = INDEX_NONE;
   return false;
}

AActor* UTATCharacterBeltComponent::GetAttachedActor(int32 slotIndex) const
{
   return _beltSlotState.IsValidIndex(slotIndex) ? _beltSlotState[slotIndex].Actor : nullptr;
}

bool UTATCharacterBeltComponent::AttachActorToBelt(AActor* existingActor, int32& outSlotIndex, FGameplayTag requiredSlotType)
{
   outSlotIndex = INDEX_NONE;
   if (_CanAttachActorToBelt(outSlotIndex, requiredSlotType))
   {
      _AttachActorToBelt(existingActor, outSlotIndex);
      return true;
   }
   return false;
}

AActor* UTATCharacterBeltComponent::SpawnAndAttachActorToBelt(TSubclassOf<AActor> actorClass, int32& outSlotIndex, FGameplayTag requiredSlotType)
{
   outSlotIndex = INDEX_NONE;
   if (_CanAttachActorToBelt(outSlotIndex, requiredSlotType))
   {
      if (AActor* newActor = GetWorld()->SpawnActor<AActor>(actorClass))
      {
         _AttachActorToBelt(newActor, outSlotIndex);
         return newActor;
      }
   }
   return nullptr;
}

AActor* UTATCharacterBeltComponent::DetachActorFromBelt(int32 slotIndex)
{
   if (_beltSlotState.IsValidIndex(slotIndex) && _beltSlotState[slotIndex].Actor != nullptr)
   {
      AActor* actor = _beltSlotState[slotIndex].Actor;
      check(actor != nullptr);
      actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
      _beltSlotState[slotIndex].Actor = nullptr;
      return actor;
   }
   return nullptr;
}

bool UTATCharacterBeltComponent::DetachAndDestroyActorFromBelt(int32 slotIndex)
{
   if (AActor* actor = DetachActorFromBelt(slotIndex))
   {
      return actor->Destroy();
   }
   return false;
}

bool UTATCharacterBeltComponent::_CanAttachActorToBelt(int32& outOpenSlotIndex, FGameplayTag requiredSlotType) const
{
   // Make sure we have a character to attach to
   ACharacter* owner = Cast<ACharacter>(GetOwner());
   if (owner == nullptr || owner->GetMesh() == nullptr)
   {
      return false;
   }

   // Find an open slot that will accept the specified slot type tag
   check(_beltSlotState.Num() == BeltAttachmentSlots.Num());
   outOpenSlotIndex = INDEX_NONE;
   for (int32 i = 0; i < _beltSlotState.Num(); i++)
   {
      if (_beltSlotState[i].Actor == nullptr && BeltAttachmentSlots[i].IsValidForType(requiredSlotType))
      {
         outOpenSlotIndex = i;
         break;
      }
   }

   return outOpenSlotIndex != INDEX_NONE;
}

void UTATCharacterBeltComponent::_AttachActorToBelt(AActor* actor, int32 slotIndex)
{
   check(BeltAttachmentSlots.IsValidIndex(slotIndex));

   ACharacter* owner = Cast<ACharacter>(GetOwner());
   check(owner != nullptr);

   USkeletalMeshComponent* characterMesh = owner->GetMesh();
   check(characterMesh != nullptr);

   const FTATBeltAttachmentSlot& slotInfo = BeltAttachmentSlots[slotIndex];
   FTATBeltAttachedActorState& slotState = _beltSlotState[slotIndex];
   check(slotState.Actor == nullptr);

   if (slotInfo.OverrideRelativeActorLocation)
   {
      // This is just for debugging and development purposes
      UCapsuleComponent* capsuleComp = owner->GetCapsuleComponent();
      check(capsuleComp != nullptr);
      actor->AttachToComponent(capsuleComp, FAttachmentTransformRules::KeepWorldTransform);
      actor->SetActorRelativeLocation(slotInfo.RelativeActorLocation);
      actor->SetActorRelativeRotation(FRotator::ZeroRotator);
   }
   else
   {
      actor->AttachToComponent(characterMesh, FAttachmentTransformRules::SnapToTargetIncludingScale, slotInfo.CharacterMeshSocket);

      if (slotInfo.UseSocketRelativeTransform)
      {
         actor->SetActorRelativeTransform(slotInfo.SocketRelativeTransform);
      }
   }

   slotState.Actor = actor;
}
