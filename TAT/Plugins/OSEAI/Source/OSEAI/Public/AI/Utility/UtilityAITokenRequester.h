// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "AI/Utility/UtilityAITokenOwner.h"

// ue4
#include "CoreMinimal.h"
#include "UObject/Interface.h"

#include "UtilityAITokenRequester.generated.h"

DECLARE_LOG_CATEGORY_EXTERN(LogUtilityAITokenRequester, Log, All);

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Request Handle
//---------------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct FFOSEAITokenRequestHandle
{
   GENERATED_BODY()

public:
   FFOSEAITokenRequestHandle() = default;

   static FFOSEAITokenRequestHandle kInvalidHandle;

   bool IsValid() const { return _handle != INDEX_NONE; }
   void Generate()
   {
      _handle = _sCurrentHandle;
      ++_sCurrentHandle;
   }

   FORCEINLINE bool operator==(const FFOSEAITokenRequestHandle& other) const { return _handle == other._handle; }

private:
   uint64 _handle = INDEX_NONE;

private:
   static uint64 _sCurrentHandle;
};

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Request
//---------------------------------------------------------------------------------------------------------

USTRUCT()
struct OSEAI_API FOSEAITokenRequest
{
   GENERATED_BODY()

public:
   FOSEAITokenRequest() = default;
   FOSEAITokenRequest(const TWeakObjectPtr<UObject>& tokenOwner, const FOSEAITokenInfo& token)
      : TokenOwner(tokenOwner)
      , Token(token)
   {

   }

   bool IsValid() const { return TokenOwner.IsValid() && Token.IsValid();  }
   FFOSEAITokenRequestHandle GenerateHandle() { Handle.Generate(); return Handle; }

   FORCEINLINE bool operator==(const FOSEAITokenRequest& other) const
   {
      return TokenOwner == other.TokenOwner &&
             Token == other.Token;
   }
   
   TWeakObjectPtr<UObject> TokenOwner = nullptr;
   FOSEAITokenInfo Token;
   FFOSEAITokenRequestHandle Handle;
};

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Requester
//---------------------------------------------------------------------------------------------------------

UCLASS(BlueprintType)
class OSEAI_API UUtilityAITokenRequester : public UObject
{
   GENERATED_BODY()

public:
   /// Create and init 
   static UUtilityAITokenRequester* Create(UObject* owner);

   /// Request a token with an expiration time
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual FFOSEAITokenRequestHandle RequestAccessToAIToken(UObject* fromTokenOwner, const FOSEAITokenInfo& token);

   /// Cancel a token request by handle when we decide we no longer need access to it
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual bool CancelAITokenRequest(FFOSEAITokenRequestHandle requestHandle);

   // Find an handle for a request.  Returns invalid handle if it's not found
   virtual FFOSEAITokenRequestHandle FindAITokenRequest(UObject* fromTokenOwner, const FOSEAITokenInfo& token) const;

   /// Do we have an open token request?
   virtual bool HasAITokenRequest(UObject* fromTokenOwner, const FOSEAITokenInfo& token) const { return FindAITokenRequest(fromTokenOwner, token).IsValid(); }

private:
   TArray<FOSEAITokenRequest> _tokenRequests;
};

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Requester Interface
/// - Implement on objects that can find/lookup a token requester
//---------------------------------------------------------------------------------------------------------

UINTERFACE(BlueprintType, MinimalAPI, Category = "AI|OSE|Utility", meta = (CannotImplementInterfaceInBlueprint))
class UUtilityAITokenRequesterInterface : public UInterface
{
   GENERATED_BODY()
};

class OSEAI_API IUtilityAITokenRequesterInterface
{
   GENERATED_BODY()

public:
   /// Returns the token requester object
   UFUNCTION(BlueprintCallable, Category = "AI|OSE|Utility", BlueprintAuthorityOnly)
   virtual UUtilityAITokenRequester* AuthorityGetTokenRequester() const = 0;
};
