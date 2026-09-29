// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Containers/ArrayView.h"
#include "UObject/Interface.h"

#include "UtilityAITokenOwner.generated.h"

struct FUtilityStateTarget;

DECLARE_LOG_CATEGORY_EXTERN(LogUtilityAITokenOwner, Log, All);

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Info
/// - Wraps a token tag / count for easier blueprint/actor properties
//---------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSEAI_API FOSEAITokenInfo
{
   GENERATED_BODY()

public:
   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (Categories = "AI.Token"), Category = "AI|OSE|Utility")
   FGameplayTag TokenTag;

   UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 0, UIMin = 0), Category = "AI|OSE|Utility")
   int Count = 1;

   bool IsValid() const { return TokenTag.IsValid() && Count > 0; }

   FORCEINLINE bool operator==(const FOSEAITokenInfo& other) const
   {
      return TokenTag == other.TokenTag && Count == other.Count;
   }
};

//---------------------------------------------------------------------------------------------------------
/// Utility AI Take Transaction Result
/// - Info returned about the token we have just attempted to take
//---------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSEAI_API FOSEAITakeTransactionResult
{
   GENERATED_BODY()

public:

   UPROPERTY(Transient, BlueprintReadOnly)
   bool Success = false;

   UPROPERTY(Transient, BlueprintReadOnly)
   bool WasDebtToken = false;
};

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Owner 
//---------------------------------------------------------------------------------------------------------

UCLASS(Abstract, BlueprintType)
class OSEAI_API UUtilityAITokenOwner : public UObject
{
   GENERATED_BODY()

public:   
   // static try get
   static UUtilityAITokenOwner* AuthorityTryGetTokenOwnerFromObject(UObject* object);

   /// Grant some tokens
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual void AuthorityGrantAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) PURE_VIRTUAL(UUtilityAITokenOwner::AuthorityGrantAITokens, );

   /// Take some tokens, returns a struct with info about the transaction attempt
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual FOSEAITakeTransactionResult AuthorityTakeAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount)  PURE_VIRTUAL(UUtilityAITokenOwner::AuthorityTakeAITokens, return FOSEAITakeTransactionResult(););

   /// Do we have the requested number of tokens?
   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual bool AuthorityHasAITokens(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) const PURE_VIRTUAL(UUtilityAITokenOwner::AuthorityHasAITokens, return false;);

   /// How many tokens do we have of this type?
   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual int AuthorityGetTokenCount(const FUtilityStateTarget& target, const FGameplayTag& tokenTag) const PURE_VIRTUAL(UUtilityAITokenOwner::AuthorityGetTokenCount, return 0;);

   /// Are we in token debt?
   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual bool AuthorityIsInTokenDebt(const FUtilityStateTarget& target, const FGameplayTag& tokenTag) const PURE_VIRTUAL(UUtilityAITokenOwner::AuthorityIsInTokenDebt, return false;);

   /// Can we go into token debt by this amount?
   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual bool AuthorityCanGoIntoTokenDebt(const FUtilityStateTarget& target, const FGameplayTag& tokenTag, int tokenCount) const PURE_VIRTUAL(UUtilityAITokenOwner::AuthorityCanGoIntoTokenDebt, return false;);

   /// Grant some tokens from token info
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual void AuthorityGrantAITokenInfo(const FUtilityStateTarget& target, const FOSEAITokenInfo& tokenInfo) { AuthorityGrantAITokens(target, tokenInfo.TokenTag, tokenInfo.Count); }

   /// Grant some tokens from an array of token info
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual void AuthorityGrantAITokenInfos(const FUtilityStateTarget& target, const TArray<FOSEAITokenInfo>& tokenInfos)
   {
      for(const FOSEAITokenInfo& tokenInfo : tokenInfos)
         AuthorityGrantAITokenInfo(target, tokenInfo);
   }

   /// Take some tokens, returns false if not all of the requested count could be taken
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual FOSEAITakeTransactionResult AuthorityTakeAITokenInfo(const FUtilityStateTarget& target, const FOSEAITokenInfo& tokenInfo) { return AuthorityTakeAITokens(target, tokenInfo.TokenTag, tokenInfo.Count); }

   /// Do we have at least one of this token?
   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual bool AuthorityHasAIToken(const FUtilityStateTarget& target, const FGameplayTag& tokenTag) const { return AuthorityHasAITokens(target, tokenTag, 1); }

   /// Do we have the token info required?
   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual bool AuthorityHasAITokenInfo(const FUtilityStateTarget& target, const FOSEAITokenInfo& tokenInfo) const { return AuthorityHasAITokens(target, tokenInfo.TokenTag, tokenInfo.Count); }

   /// Are we in token debt?
   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual bool AuthorityIsInTokenDebtFromInfo(const FUtilityStateTarget& target, const FOSEAITokenInfo& tokenInfo) const { return AuthorityIsInTokenDebt(target, tokenInfo.TokenTag); }

   /// Can we go into token debt by this token info amount?
   UFUNCTION(BlueprintPure, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual bool AuthorityCanGoIntoTokenDebtFromInfo(const FUtilityStateTarget& target, const FOSEAITokenInfo& tokenInfo) const { return AuthorityCanGoIntoTokenDebt(target, tokenInfo.TokenTag, tokenInfo.Count); }

protected:
   FString _GetDebugName() const;
};

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Owner Interface
/// - Implement on objects that can find/lookup a token owner
//---------------------------------------------------------------------------------------------------------

// Exposed to blueprints; required for reflection. Not the actual interface type.
UINTERFACE(BlueprintType, MinimalAPI, Category = "AI|OSE|Utility")
class UUtilityAITokenOwnerInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEAI_API IUtilityAITokenOwnerInterface
{
   GENERATED_BODY()

public:
   /// Returns the token owner object
   UFUNCTION(BlueprintNativeEvent, BlueprintCallable, BlueprintAuthorityOnly, Category = "AI|OSE|Utility")
   UUtilityAITokenOwner* AuthorityGetTokenOwner() const;
};
