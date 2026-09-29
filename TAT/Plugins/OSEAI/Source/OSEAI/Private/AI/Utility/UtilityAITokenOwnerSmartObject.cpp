// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Utility/UtilityAITokenOwnerSmartObject.h"

// ose
#include "AI/OSEAISettings.h"

// ue
#include "SmartObjectComponent.h"
#include "SmartObjectSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAITokenOwnerSmartObject)

DEFINE_LOG_CATEGORY(LogUtilityAITokenOwnerSmartObject);

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Owner Smart Object
//---------------------------------------------------------------------------------------------------------

/* static */
UUtilityAITokenOwnerSmartObject* UUtilityAITokenOwnerSmartObject::AuthorityCreate(USmartObjectComponent* ownerSmartObjectComponent)
{
   check(ownerSmartObjectComponent);
   UUtilityAITokenOwnerSmartObject* tokenOwner = NewObject<UUtilityAITokenOwnerSmartObject>(ownerSmartObjectComponent);
   check(tokenOwner);
   tokenOwner->_world = ownerSmartObjectComponent->GetWorld();
   return tokenOwner;
}

/* static */
UUtilityAITokenOwnerSmartObject* UUtilityAITokenOwnerSmartObject::BP_AuthorityCreateTokenOwnerSmartObject(USmartObjectComponent* ownerSmartObjectComponent)
{
   return UUtilityAITokenOwnerSmartObject::AuthorityCreate(ownerSmartObjectComponent);
}

void UUtilityAITokenOwnerSmartObject::AuthorityGrantAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount)
{
   check(_IsValidSmartObjectTarget(target));

   UE_LOG(LogUtilityAITokenOwnerSmartObject, Verbose, TEXT("%s: %d AI tokens of type %s granted (total count: %d)"), *_GetDebugName(), tokenCount, *tokenTag.ToString(), tokenCount);

   FSmartObjectClaimHandle& claimHandle = _claimHandles.FindOrAdd(target.SmartObjectRequestTarget.SlotHandle);
   if (claimHandle.IsValid())
   {
      UE_LOG(LogUtilityAITokenOwnerSmartObject, Verbose, TEXT("   - Claim handle is valid."));

      if (USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld()))
      {
         const ESmartObjectSlotState slotState = smartObjectSubsystem->GetSlotState(target.SmartObjectRequestTarget.SlotHandle);

         // tasks like UAITask_UseSmartObject can cause a smart object to be released after it's used, so we may end skipping our own release
         if (slotState == ESmartObjectSlotState::Claimed || slotState == ESmartObjectSlotState::Occupied)
         {
            bool success = smartObjectSubsystem->Release(claimHandle);

            UE_LOG(LogUtilityAITokenOwnerSmartObject, Verbose, TEXT("   - Released via claim handle. (%s)"), success ? TEXT("Succeeded") : TEXT("Already Taken"));
         }
         else
         {
            UE_LOG(LogUtilityAITokenOwnerSmartObject, Verbose, TEXT("   - Not released via claim handle, something else already released it!"));
         }
      }
      claimHandle.Invalidate();
      UE_LOG(LogUtilityAITokenOwnerSmartObject, Verbose, TEXT("   - Invalidated the claim handle"));
   }
}

FOSEAITakeTransactionResult UUtilityAITokenOwnerSmartObject::AuthorityTakeAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount)
{
   check(_IsValidSmartObjectTarget(target));

   // we should only be taking tokens from smart object targets
   check(target.TargetType == EBehaviorTargetType::SmartObjectRequest);
   check(target.SmartObjectRequestTarget.IsValid());

   UE_LOG(LogUtilityAITokenOwnerSmartObject, Verbose, TEXT("%s: Take %d AI tokens of type %s"), *_GetDebugName(), tokenCount, *tokenTag.ToString());

   FOSEAITakeTransactionResult result;

   FSmartObjectClaimHandle& claimHandle = _claimHandles.FindOrAdd(target.SmartObjectRequestTarget.SlotHandle);

   // we should not have a previous claim handle
   check(!claimHandle.IsValid());

   // try to claim the smart object request
   if (USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld()))
   {
      const ESmartObjectSlotState slotState = smartObjectSubsystem->GetSlotState(target.SmartObjectRequestTarget.SlotHandle);
      if (slotState == ESmartObjectSlotState::Free)
      {
         claimHandle = smartObjectSubsystem->MarkSlotAsClaimed(target.SmartObjectRequestTarget.SlotHandle, ESmartObjectClaimPriority::Normal);
      }

      // successful if it was free and we claimed it
      result.Success = claimHandle.IsValid();
   }

   if (result.Success)
   {
      UE_LOG(LogUtilityAITokenOwnerSmartObject, Verbose, TEXT("   - Successfully claimed the smart object request target"));
   }
   else
   {
      UE_LOG(LogUtilityAITokenOwnerSmartObject, Verbose, TEXT("   - Failed to claim the smart object request target"));
   }

   return result;
}

bool UUtilityAITokenOwnerSmartObject::AuthorityHasAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) const
{
   check(_IsValidSmartObjectTarget(target));

   USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(GetWorld());
   if (!smartObjectSubsystem)
      return false;
   
   // if we have a valid claim handle for this slot, we do not "have a token"
   // if we don't know anything about this slot, we do "have a token"
   bool hasToken = true;
   
   if (const FSmartObjectClaimHandle* claimHandle = _claimHandles.Find(target.SmartObjectRequestTarget.SlotHandle))
   {
      hasToken = !claimHandle->IsValid();
   }
   
   UE_LOG(LogUtilityAITokenOwnerSmartObject, Verbose, TEXT("%s: AI %s %d tokens of type %s"), *_GetDebugName(), hasToken ? TEXT("has") : TEXT("does NOT have"), tokenCount, *tokenTag.ToString());

   return hasToken;
}

int UUtilityAITokenOwnerSmartObject::AuthorityGetTokenCount(const FUtilityStateTarget& target, const FGameplayTag& tokenTag) const
{
   check(_IsValidSmartObjectTarget(target));

   // we only have 1 or 0 (free or not)
   const int tokenCount = AuthorityHasAITokens(target, tokenTag, 1) ? 1 : 0;

   UE_LOG(LogUtilityAITokenOwnerSmartObject, Verbose, TEXT("%s: AI has %d tokens of type %s"), *_GetDebugName(), tokenCount, *tokenTag.ToString());

   return tokenCount;
}

bool UUtilityAITokenOwnerSmartObject::AuthorityIsInTokenDebt(const FUtilityStateTarget& target, const FGameplayTag& tokenTag) const
{
   check(_IsValidSmartObjectTarget(target));

   // no token debt allowed for smart objects
   return false;
}

bool UUtilityAITokenOwnerSmartObject::AuthorityCanGoIntoTokenDebt(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) const
{
   check(_IsValidSmartObjectTarget(target));

   // no token debt allowed for smart objects
   return false;
}

FSmartObjectClaimHandle UUtilityAITokenOwnerSmartObject::GetClaimHandle(const FUtilityStateTarget& target) const
{
   if(_IsValidSmartObjectTarget(target))
   {
      if (const FSmartObjectClaimHandle* claimHandle = _claimHandles.Find(target.SmartObjectRequestTarget.SlotHandle))
      {
         return *claimHandle;
      }
   }
   return FSmartObjectClaimHandle::InvalidHandle;
}

bool UUtilityAITokenOwnerSmartObject::_IsValidSmartObjectTarget(const FUtilityStateTarget& target) const
{
   return target.TargetType == EBehaviorTargetType::SmartObjectRequest && target.SmartObjectRequestTarget.IsValid();
}

