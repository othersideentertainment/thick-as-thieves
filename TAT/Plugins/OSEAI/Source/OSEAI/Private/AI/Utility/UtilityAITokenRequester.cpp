// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#include "AI/Utility/UtilityAITokenRequester.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAITokenRequester)

DEFINE_LOG_CATEGORY(LogUtilityAITokenRequester);

uint64 FFOSEAITokenRequestHandle::_sCurrentHandle = 0;
FFOSEAITokenRequestHandle FFOSEAITokenRequestHandle::kInvalidHandle;

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Requester
//---------------------------------------------------------------------------------------------------------

/* static */
UUtilityAITokenRequester* UUtilityAITokenRequester::Create(UObject* owner)
{
   check(owner);
   return NewObject<UUtilityAITokenRequester>(owner);
}

FFOSEAITokenRequestHandle UUtilityAITokenRequester::RequestAccessToAIToken(UObject* fromTokenOwner, const FOSEAITokenInfo& token)
{
   FOSEAITokenRequest req = { fromTokenOwner, token };

   if (!req.IsValid())
   {
      UE_LOG(LogUtilityAITokenRequester, Warning, TEXT("Invalid AI token request. Token Owner: %s, Token %s")
         , fromTokenOwner ? *fromTokenOwner->GetName() : TEXT("None")
         , *token.TokenTag.ToString());
      return FFOSEAITokenRequestHandle::kInvalidHandle;
   }

   // check to see if we already have a request
   if (FOSEAITokenRequest* foundReq = _tokenRequests.FindByKey(req))
   {
      check(foundReq->Handle.IsValid());
      return foundReq->Handle;
   }

   // or create a new one
   FFOSEAITokenRequestHandle handle = req.GenerateHandle();
   _tokenRequests.Emplace(req);
   return handle;
}

bool UUtilityAITokenRequester::CancelAITokenRequest(FFOSEAITokenRequestHandle requestHandle)
{
   if (!requestHandle.IsValid())
      return false;

   for (auto it = _tokenRequests.CreateIterator(); it; ++it)
   {
      const FOSEAITokenRequest& tokenReq = (*it);
      if (tokenReq.Handle == requestHandle)
      {
         it.RemoveCurrent();
         return true;
      }
   }

   return false;
}

FFOSEAITokenRequestHandle UUtilityAITokenRequester::FindAITokenRequest(UObject* fromTokenOwner, const FOSEAITokenInfo& token) const
{
   if (fromTokenOwner && token.IsValid())
   {
      for (const FOSEAITokenRequest& tokenReq : _tokenRequests)
      {
         if (tokenReq.TokenOwner == fromTokenOwner && tokenReq.Token == token)
         {
            check(tokenReq.Handle.IsValid());
            return tokenReq.Handle;
         }
      }
   }
   return FFOSEAITokenRequestHandle::kInvalidHandle;
}

