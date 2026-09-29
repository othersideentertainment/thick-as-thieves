// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/UtilityAITokenOwner.h"

// ue4
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

#include "UtilityAITokenOwnerGameplayTagCount.generated.h"

class USmartObjectComponent;

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Gameplay Tag Count
//---------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class OSEAI_API UUtilityAITokenOwnerGameplayTagCount : public UUtilityAITokenOwner
{
   GENERATED_BODY()

public:
   /// Create and init 
   static UUtilityAITokenOwnerGameplayTagCount* AuthorityCreate(UObject* owner, const TArray<FOSEAITokenInfo>& defaultTokens, const TArray<FOSEAITokenInfo>& maxTokenDebt);
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "AI|OSE|Utility", DisplayName = "AuthorityCreateTokenOwnerGameplayTagCount")
   static UUtilityAITokenOwnerGameplayTagCount* BP_AuthorityCreateTokenOwnerGameplayTagCount(UObject* owner, const TArray<FOSEAITokenInfo>& defaultTokens, const TArray<FOSEAITokenInfo>& maxTokenDebt);

   // from UUtilityAITokenOwner
   virtual void AuthorityGrantAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) override;
   virtual FOSEAITakeTransactionResult AuthorityTakeAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) override;
   virtual bool AuthorityHasAITokens(const FUtilityStateTarget & target, const FGameplayTag & tokenTag, int tokenCount) const override;
   virtual int AuthorityGetTokenCount(const FUtilityStateTarget & target, const FGameplayTag & tokenTag) const override;
   virtual bool AuthorityIsInTokenDebt(const FUtilityStateTarget & target, const FGameplayTag & tokenTag) const override;
   virtual bool AuthorityCanGoIntoTokenDebt(const FUtilityStateTarget & target, const FGameplayTag & tokenTag, int tokenCount) const override;

protected:
   void _AuthorityInit(const TArray<FOSEAITokenInfo>& defaultTokens, const TArray<FOSEAITokenInfo>& maxTokenDebt);

private:
   int _GetCurrentTagCount(const FGameplayTag& tokenTag) const;
   void _CheckValidTokenCount(const FGameplayTag& tokenTag) const;
   int _GetTokenCountMin(const FGameplayTag& tokenTag) const;
   int _GetTokenCountMax(const FGameplayTag& tokenTag) const;

private:
   TMap<FGameplayTag, int> _currentTagCount;
   TMap<FGameplayTag, int> _defaultTagCounts;
   TMap<FGameplayTag, int> _maxTagDebts;
};
