// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ItemToolComponent.h"

// ose
#include "Items/ItemInfo.h"

// ue4
#include "Paper2D/Classes/PaperSprite.h"
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ItemToolComponent)

UItemToolComponent::UItemToolComponent()
   : _copyBasicItemInfoToToolInfo(true)
   , _injectItemNameInInputPrompts(false)
{
}

bool UItemToolComponent::IsReady() const
{
   if (!Super::IsReady())
   {
      return false;
   }

   // wait for granted item class to be replicated
   return _grantedByItemInfoClass != nullptr;
}

void UItemToolComponent::OnRegister()
{
   Super::OnRegister();

   if (GetOwner()->HasAuthority() && ensure(_grantedByItemInfoClass))
   {
      _OnItemInfoAvailable();
   }
}

void UItemToolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);
   DOREPLIFETIME_CONDITION(UItemToolComponent, _grantedByItemInfoClass, COND_InitialOnly);
}

#if WITH_EDITOR
bool UItemToolComponent::CanEditChange(const FProperty* inProperty) const
{
   if (!Super::CanEditChange(inProperty))
   {
      return false;
   }

   if ((inProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FOSEAbilityInfo, ToolName)) ||
       (inProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FOSEAbilityInfo, ToolDescription)) ||
       (inProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FOSEAbilityInfo, SourceSprite)))
   {
      return !_copyBasicItemInfoToToolInfo;
   }

   return true;
}
#endif

void UItemToolComponent::_OnRep_GrantedByItemInfoClass()
{
   if (_grantedByItemInfoClass)
   {
      _OnItemInfoAvailable();
   }
}

void UItemToolComponent::_OnItemInfoAvailable()
{
   // could technically do this only if local controlled, but this straight copy should be relatively cheap
   if (_copyBasicItemInfoToToolInfo)
   {
      const UItemInfo* itemCdo = _grantedByItemInfoClass.GetDefaultObject();
      ToolInfo.ToolName = itemCdo->Name;
      ToolInfo.ToolDescription = itemCdo->Description;
      // NB: This is converting from a direct pointer to a soft reference, which requires the full class definition
      ToolInfo.SourceSprite = itemCdo->Icon;
   }

   // NB: Local control _should_ be knowable by now for item tools,
   //     since the inventory granting the tool implies possession,
   //     for players at least
   if (_injectItemNameInInputPrompts && GetOwnerCharacter()->IsLocallyControlled())
   {
      for (FOSEToolInputInfo& inputInfo : ToolInput.InputInfo)
      {
         if (FTextInspector::GetDisplayString(inputInfo.InputText).Contains(TEXT("{ItemName}")))
         {
            const UItemInfo* itemCdo = _grantedByItemInfoClass.GetDefaultObject();
            inputInfo.InputText = FText::FormatNamed(inputInfo.InputText, TEXT("ItemName"), itemCdo->Name);
         }
      }
   }

   _ModifyAllVisuals();
}

