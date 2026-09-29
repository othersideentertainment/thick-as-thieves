// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tools/TATLargeCarryLootToolComponent.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Loot/TATLootInventory.h"

// ose
#include "Items/ToolSetComponent.h"

// ue5
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATLargeCarryLootToolComponent)

UTATLargeCarryLootToolComponent::UTATLargeCarryLootToolComponent()
{
   // Still make component, since the mesh will be set later
   ToolVisuals1P.SkipComponentWithoutMesh = false;
   ToolVisuals3P.SkipComponentWithoutMesh = false;
}

bool UTATLargeCarryLootToolComponent::IsReady() const
{
   return Super::IsReady() && _lootParams.IsValid();
}

UTATLootInventoryComponent* UTATLargeCarryLootToolComponent::GetOwningInventory() const
{
   if (AActor* owner = GetOwner())
   {
      UTATLootInventoryComponent* inventory = UTATLootInventoryComponent::GetForActor(GetOwner());
      if (IsValid(inventory))
      {
         return inventory;
      }
   }
   return nullptr;
}

void UTATLargeCarryLootToolComponent::OnRegister()
{
   Super::OnRegister();

   if (_HasAuthority() && ensure(_lootParams.IsValid()))
   {
      _OnLootParamsAvailable();
   }
}

void UTATLargeCarryLootToolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(UTATLargeCarryLootToolComponent, _lootParams, COND_InitialOnly);
}

bool UTATLargeCarryLootToolComponent::OnAddToToolSet_Implementation()
{
   Super::OnAddToToolSet_Implementation();

   if (_automaticallyEquipOnAddToToolset)
   {
      if (UToolSetComponent* toolsetComponent = _GetOwningToolSet())
      {
         toolsetComponent->EquipToolByClass(GetClass());
      }
   }

   return true;
}

bool UTATLargeCarryLootToolComponent::OnUnequip_Implementation()
{
   const bool result = Super::OnUnequip_Implementation();

   // This is not expected to be the primary way this happens, but
   // auto-drop the loot if unequipped to prevent it from getting
   // into a state where the tool is un-equipped, but the loot still
   // held.
   if (GetOwner() && GetOwner()->HasAuthority())
   {
      // NOTE: UTATToolSetComponent::_OnToolRemoved makes for some potential weirdness
      UTATLootInventoryComponent* inventory = UTATLootInventoryComponent::GetForActor(GetOwner());
      if (IsValid(inventory) && inventory->GetLargeCarryLootId() == _lootParams.LootId)
      {
         inventory->AuthorityDropLargeCarryLootVoluntarily();
      }
   }

   return result;
}

#if WITH_EDITOR
bool UTATLargeCarryLootToolComponent::CanEditChange(const FProperty* inProperty) const
{
   if (!Super::CanEditChange(inProperty))
   {
      return false;
   }

   if ((inProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FOSEAbilityInfo, ToolName)) ||
      (inProperty->GetFName() == GET_MEMBER_NAME_CHECKED(FOSEAbilityInfo, ToolDescription)))
   {
      return false;
   }

   return true;
}
#endif

bool UTATLargeCarryLootToolComponent::_HasAuthority() const
{
   if (AActor* owner = GetOwner())
   {
      return owner->HasAuthority();
   }
   return false;
}

void UTATLargeCarryLootToolComponent::_OnRep_LootParams()
{
   if (_lootParams.IsValid())
   {
      _OnLootParamsAvailable();
   }
}

void UTATLargeCarryLootToolComponent::_OnLootParamsAvailable()
{
   ensureAlways(_lootParams.IsValid());

   const FTATLootInfo* lootInfo = UTATLootSettings::Get().GetLootInfo(this, _lootParams.LootId);
   if (!ensure(lootInfo))
   {
      return;
   }

   ToolInfo.ToolName = lootInfo->DisplayName;
   _InjectIntoInputPrompts(*lootInfo);

   auto updateMesh = [this](FToolVisuals& visuals)
   {
      if (visuals.MeshComponent)
      {
         visuals.MeshComponent->SetSkeletalMeshAsset(_lootParams.Mesh);
      }
      else
      {
         visuals.MeshAsset = _lootParams.Mesh;
      }
   };

   // explicitly skipping the unequipped mesh
   updateMesh(ToolVisuals1P);
   updateMesh(ToolVisuals3P);
}

void UTATLargeCarryLootToolComponent::_InjectIntoInputPrompts(const FTATLootInfo& lootInfo)
{
   if (GetOwnerCharacter()->IsLocallyControlled())
   {
      for (FOSEToolInputInfo& inputInfo : ToolInput.InputInfo)
      {
         if (FTextInspector::GetDisplayString(inputInfo.InputText).Contains(TEXT("{LootName}")))
         {
            inputInfo.InputText = FText::FormatNamed(inputInfo.InputText, TEXT("LootName"), lootInfo.DisplayName);
         }
      }
   }
}
