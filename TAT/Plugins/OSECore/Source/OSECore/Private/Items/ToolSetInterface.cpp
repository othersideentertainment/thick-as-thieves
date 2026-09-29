// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ToolSetInterface.h"

// ose
#include "Items/ToolSetSystemInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolSetInterface)

// Add default functionality here for any IToolSetInterface functions that are not pure virtual.

TScriptInterface<IToolSetInterface> IToolSetInterface::GetToolSetFromActor(AActor* actor)
{
   if (IToolSetSystemInterface* toolSetSystem = Cast<IToolSetSystemInterface>(actor))
   {
      return toolSetSystem->GetToolSetInterface();
   }

   return nullptr;
}
