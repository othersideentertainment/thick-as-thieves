// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Utility/UtilityAITokenOwnerGameplayTagCount.h"

// ose
#include "AI/OSEAISettings.h"
#include "AI/Utility/UtilityAIStateTarget.h"

// ue
#include "SmartObjectComponent.h"
#include "SmartObjectSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAITokenOwnerGameplayTagCount)

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Gameplay Tag Count
//---------------------------------------------------------------------------------------------------------

/* static */
UUtilityAITokenOwnerGameplayTagCount* UUtilityAITokenOwnerGameplayTagCount::AuthorityCreate(UObject* owner, const TArray<FOSEAITokenInfo>& defaultTokens, const TArray<FOSEAITokenInfo>& maxTokenDebt)
{
   check(owner);
   UUtilityAITokenOwnerGameplayTagCount* tokenOwner = NewObject<UUtilityAITokenOwnerGameplayTagCount>(owner);
   check(tokenOwner);
   tokenOwner->_AuthorityInit(defaultTokens, maxTokenDebt);
   return tokenOwner;
}

/* static */
UUtilityAITokenOwnerGameplayTagCount* UUtilityAITokenOwnerGameplayTagCount::BP_AuthorityCreateTokenOwnerGameplayTagCount(UObject* owner, const TArray<FOSEAITokenInfo>& defaultTokens, const TArray<FOSEAITokenInfo>& maxTokenDebt)
{
   return UUtilityAITokenOwnerGameplayTagCount::AuthorityCreate(owner, defaultTokens, maxTokenDebt);
}

void UUtilityAITokenOwnerGameplayTagCount::AuthorityGrantAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount)
{
   check(tokenCount > 0);
   if (tokenTag.IsValid())
   {
      int& count = _currentTagCount.FindOrAdd(tokenTag);
      count = count + tokenCount;
      _CheckValidTokenCount(tokenTag);
      UE_LOG(LogUtilityAITokenOwner, Verbose, TEXT("%s: %d AI tokens of type %s granted (total count: %d)"), *_GetDebugName(), tokenCount, *tokenTag.ToString(), count);
   }
}

FOSEAITakeTransactionResult UUtilityAITokenOwnerGameplayTagCount::AuthorityTakeAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount)
{
   FOSEAITakeTransactionResult ret;
   check(tokenCount > 0);
   if (AuthorityHasAITokens(target, tokenTag, tokenCount) || AuthorityCanGoIntoTokenDebt(target, tokenTag, tokenCount))
   {
      int& count = _currentTagCount.FindOrAdd(tokenTag);
      count = count - tokenCount;
      _CheckValidTokenCount(tokenTag);
      ret.Success = true;
      ret.WasDebtToken = count < 0;
      UE_LOG(LogUtilityAITokenOwner, Verbose, TEXT("%s: %d AI tokens of type %s taken  (total count: %d)"), *_GetDebugName(), tokenCount, *tokenTag.ToString(), count);
   }
   return ret;
}

bool UUtilityAITokenOwnerGameplayTagCount::AuthorityHasAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) const
{
   const bool hasTokens = _GetCurrentTagCount(tokenTag) >= tokenCount;
   UE_LOG(LogUtilityAITokenOwner, Verbose, TEXT("%s: AI %s %d tokens of type %s"), *_GetDebugName(), hasTokens ? TEXT("has") : TEXT("does NOT have"), tokenCount, *tokenTag.ToString());
   return hasTokens;
}

int UUtilityAITokenOwnerGameplayTagCount::AuthorityGetTokenCount(const FUtilityStateTarget& target, const FGameplayTag& tokenTag) const
{
   const int tokenCount = _GetCurrentTagCount(tokenTag);
   UE_LOG(LogUtilityAITokenOwner, Verbose, TEXT("%s: AI has %d tokens of type %s"), *_GetDebugName(), tokenCount, *tokenTag.ToString());
   return tokenCount;
}

bool UUtilityAITokenOwnerGameplayTagCount::AuthorityIsInTokenDebt(const FUtilityStateTarget& target, const FGameplayTag& tokenTag) const
{
   const int currentTokenCount = _GetCurrentTagCount(tokenTag);
   const bool isInDebt = currentTokenCount < 0;
   UE_LOG(LogUtilityAITokenOwner, Verbose, TEXT("%s: AI is %s debt for tokens of type %s (current count: %d)"), *_GetDebugName(), isInDebt ? TEXT("in") : TEXT("NOT in"), *tokenTag.ToString(), currentTokenCount);
   return isInDebt;
}

bool UUtilityAITokenOwnerGameplayTagCount::AuthorityCanGoIntoTokenDebt(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) const
{
   const int currentTokenCount = _GetCurrentTagCount(tokenTag);
   const int debtCount = currentTokenCount - tokenCount;
   const int tokenCountMin = _GetTokenCountMin(tokenTag);
   const bool canGoIntoDebt = debtCount >= tokenCountMin;
   UE_LOG(LogUtilityAITokenOwner, Verbose, TEXT("%s: AI %s go into debt for %d tokens of type %s (current count: %d)"), *_GetDebugName(), canGoIntoDebt ? TEXT("can") : TEXT("CAN NOT"), tokenCount, *tokenTag.ToString(), currentTokenCount);
   return canGoIntoDebt;
}

void UUtilityAITokenOwnerGameplayTagCount::_AuthorityInit(const TArray<FOSEAITokenInfo>& defaultTokens, const TArray<FOSEAITokenInfo>& maxTokenDebt)
{
   // initial setup: default tokens
   for (const FOSEAITokenInfo& defaultTokenInfo : defaultTokens)
   {
      int& count = _defaultTagCounts.FindOrAdd(defaultTokenInfo.TokenTag);
      count += defaultTokenInfo.Count;
   }

   // initial setup: token debt
   for (const FOSEAITokenInfo& tokenDebtInfo : maxTokenDebt)
   {
      int& count = _maxTagDebts.FindOrAdd(tokenDebtInfo.TokenTag);
      count += tokenDebtInfo.Count;
   }

   // grant tokens
   AuthorityGrantAITokenInfos(FUtilityStateTarget::Invalid, defaultTokens);
}

int UUtilityAITokenOwnerGameplayTagCount::_GetCurrentTagCount(const FGameplayTag& tokenTag) const
{
   if (const int* tagCount = _currentTagCount.Find(tokenTag))
   {
      return *tagCount;
   }
   return 0;
}

void UUtilityAITokenOwnerGameplayTagCount::_CheckValidTokenCount(const FGameplayTag& tokenTag) const
{
   const int currentCount = _GetCurrentTagCount(tokenTag);
   const int minCount = _GetTokenCountMin(tokenTag);
   const int maxCount = _GetTokenCountMax(tokenTag);
   check(minCount <= currentCount && currentCount <= maxCount);
}

int UUtilityAITokenOwnerGameplayTagCount::_GetTokenCountMin(const FGameplayTag& tokenTag) const
{
   if (const int* maxDebtCount = _maxTagDebts.Find(tokenTag))
   {
      return -(*maxDebtCount);
   }
   return 0;
}

int UUtilityAITokenOwnerGameplayTagCount::_GetTokenCountMax(const FGameplayTag& tokenTag) const
{
   if (const int* defaultCount = _defaultTagCounts.Find(tokenTag))
   {
      return *defaultCount;
   }
   return 0;
}

