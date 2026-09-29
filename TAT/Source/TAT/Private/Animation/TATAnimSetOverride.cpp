// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Animation/TATAnimSetOverride.h"

// ue
#include "Algo/MaxElement.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAnimSetOverride)

void FTATAnimSetOverrides::AddRequest(const FTATAnimSetRequest& request)
{
   _requests.Add(request);
}

void FTATAnimSetOverrides::RemoveRequest(const FTATAnimSetRequest& request)
{
   _requests.RemoveSingleSwap(request);
}

void FTATAnimSetOverrides::RemoveBySource(FObjectKey source)
{
   _requests.RemoveAllSwap([source](const FTATAnimSetRequest& request) { return request.Source == source; });
}

FGameplayTag FTATAnimSetOverrides::GetCurrentOverride() const
{
   if(_requests.IsEmpty())
   {
      return FGameplayTag();
   }

   const FTATAnimSetRequest* request = Algo::MaxElementBy(_requests, [](const FTATAnimSetRequest& request) { return request.Priority; });
   check(request);
   return request->AnimSetTag;
}
