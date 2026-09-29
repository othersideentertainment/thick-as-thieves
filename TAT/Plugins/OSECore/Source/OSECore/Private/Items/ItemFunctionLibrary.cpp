// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ItemFunctionLibrary.h"

// ose
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"
#include "Items/ToolComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemFunctionLibrary)

// ue4

void UOSEItemFunctionLibrary::StowCurrentToolForAvatar(const AActor* actor, bool stowed)
{
   if (auto toolSetSystemInterface = Cast<IToolSetSystemInterface>(actor))
   {
      if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystemInterface->GetToolSetInterface())
      {
         toolSetInterface->StowCurrentTool(stowed);
      }
   }
}

bool UOSEItemFunctionLibrary::CanAvatarAttackWithCurrentTool(const AActor* actor)
{
   if (auto toolSetSystemInterface = Cast<IToolSetSystemInterface>(actor))
   {
      if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystemInterface->GetToolSetInterface())
      {
         if (UToolComponent* tool = toolSetInterface->GetCurrentTool())
         {
            return tool->IsEquipped() && !tool->IsStowed();
         }
      }
   }
   return false;
}

