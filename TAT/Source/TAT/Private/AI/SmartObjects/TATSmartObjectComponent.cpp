// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// tat
#include "AI/SmartObjects/TATSmartObjectComponent.h"
#include "AI/SmartObjects/TATSmartObjectTagInterface.h"

// ue
#include "SmartObjectSubsystem.h"
#include "GameplayEffectTypes.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATSmartObjectComponent)

UTATSmartObjectComponent::UTATSmartObjectComponent(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   bWantsInitializeComponent = true;
}

void UTATSmartObjectComponent::BeginPlay()
{
   Super::BeginPlay();

   if (GetOwner()->HasAuthority())
   {
      _tokenOwner = UUtilityAITokenOwnerSmartObject::AuthorityCreate(this);
   }
}

FSmartObjectClaimHandle UTATSmartObjectComponent::TryClaim(const FSmartObjectRequestFilter& filter) const
{
   USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld());
   check(smartObjectSubsystem);

   TArray<FSmartObjectSlotHandle> slotHandles;
   smartObjectSubsystem->FindSlots(GetRegisteredHandle(), filter, slotHandles);
   if (slotHandles.IsEmpty())
   {
      return FSmartObjectClaimHandle::InvalidHandle;
   }
   return smartObjectSubsystem->MarkSlotAsClaimed(slotHandles[0], ESmartObjectClaimPriority::Normal);
}

FSmartObjectClaimHandle UTATSmartObjectComponent::TryClaim(const FSmartObjectSlotHandle& slotHandle) const
{
   USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld());
   check(smartObjectSubsystem);
   return smartObjectSubsystem->MarkSlotAsClaimed(slotHandle, ESmartObjectClaimPriority::Normal);
}

FSmartObjectClaimHandle UTATSmartObjectComponent::TryClaimWithSlotIndex(int slotIndex) const
{
   USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld());
   check(smartObjectSubsystem);
   TArray<FSmartObjectSlotHandle> slotHandles;
   smartObjectSubsystem->GetAllSlots(GetRegisteredHandle(), slotHandles);
   if (slotHandles.IsValidIndex(slotIndex))
   {
      return smartObjectSubsystem->MarkSlotAsClaimed(slotHandles[slotIndex], ESmartObjectClaimPriority::Normal);
   }
   return FSmartObjectClaimHandle::InvalidHandle;
}
