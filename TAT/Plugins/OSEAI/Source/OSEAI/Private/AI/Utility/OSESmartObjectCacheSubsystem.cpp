// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Utility/OSESmartObjectCacheSubsystem.h"

// ue5
#include "SmartObjectComponent.h"
#include "SmartObjectSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESmartObjectCacheSubsystem)

USmartObjectComponent* UOSESmartObjectCacheSubsystem::GetSmartObjectComponentForHandle(FSmartObjectHandle handle)
{
   TWeakObjectPtr<USmartObjectComponent>* found = _cachedSmartObjects.Find(handle);
   USmartObjectComponent* result = found ? found->Get() : nullptr;
   if (result == nullptr)
   {
      if (USmartObjectSubsystem* subsystem = USmartObjectSubsystem::GetCurrent(GetWorld()))
      {
         result = subsystem->GetSmartObjectComponentByHandle(handle);
         _cachedSmartObjects.Emplace(handle, result);
      }
   }

   return result;
}

