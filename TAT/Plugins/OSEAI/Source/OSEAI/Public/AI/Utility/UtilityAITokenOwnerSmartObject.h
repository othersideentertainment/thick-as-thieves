// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/UtilityAITokenOwner.h"

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "SmartObjectRuntime.h"
#include "SmartObjectTypes.h"

#include "UtilityAITokenOwnerSmartObject.generated.h"

class USmartObjectComponent;

DECLARE_LOG_CATEGORY_EXTERN(LogUtilityAITokenOwnerSmartObject, Log, All);

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Owner Smart Object
//---------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class OSEAI_API UUtilityAITokenOwnerSmartObject : public UUtilityAITokenOwner
{
   GENERATED_BODY()

public:
   /// Create and init 
   static UUtilityAITokenOwnerSmartObject* AuthorityCreate(USmartObjectComponent* smartObjectComponent);
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AI|OSE|Utility", DisplayName = "AuthorityCreateTokenOwnerSmartObject")
   static UUtilityAITokenOwnerSmartObject* BP_AuthorityCreateTokenOwnerSmartObject(USmartObjectComponent* smartObjectComponent);

   // from UUtilityAITokenOwner
   virtual void AuthorityGrantAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) override;
   virtual FOSEAITakeTransactionResult AuthorityTakeAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) override;
   virtual bool AuthorityHasAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) const override;
   virtual int AuthorityGetTokenCount(const FUtilityStateTarget& target, const FGameplayTag& tokenTag) const override;
   virtual bool AuthorityIsInTokenDebt(const FUtilityStateTarget& target, const FGameplayTag& tokenTag) const override;
   virtual bool AuthorityCanGoIntoTokenDebt(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) const override;

   UFUNCTION(BlueprintPure, BlueprintAuthorityOnly, Category = "AI|OSE|Utility")
   FSmartObjectClaimHandle GetClaimHandle(const FUtilityStateTarget& target) const;

   // from UObject
   virtual UWorld* GetWorld() const override { return _world.Get(); }

private:
   bool _IsValidSmartObjectTarget(const FUtilityStateTarget& target) const;

private:
   TWeakObjectPtr<UWorld> _world;
   TMap<FSmartObjectSlotHandle, FSmartObjectClaimHandle> _claimHandles;
};
