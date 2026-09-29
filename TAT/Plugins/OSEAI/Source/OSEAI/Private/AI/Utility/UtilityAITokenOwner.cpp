// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Utility/UtilityAITokenOwner.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAITokenOwner)

DEFINE_LOG_CATEGORY(LogUtilityAITokenOwner);

//---------------------------------------------------------------------------------------------------------
/// Utility AI Token Owner
//---------------------------------------------------------------------------------------------------------

UUtilityAITokenOwner* UUtilityAITokenOwner::AuthorityTryGetTokenOwnerFromObject(UObject* object)
{
   if (IsValid(object) && object->Implements<UUtilityAITokenOwnerInterface>())
   {
      return IUtilityAITokenOwnerInterface::Execute_AuthorityGetTokenOwner(object);
   }
   return nullptr;
}

FString UUtilityAITokenOwner::_GetDebugName() const
{
   if(const UObject* outer = GetOuter())
   {
      return outer->GetName();
   }
   return TEXT("[Unknown]");
}

