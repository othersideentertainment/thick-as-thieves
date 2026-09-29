// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tools/TATMajorLootToolComponent.h"

// tat
#include "Developer/TATLootSettings.h"
#include "Loot/TATLootInventory.h"

// ose
#include "Items/ToolSetComponent.h"

// ue5
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATMajorLootToolComponent)

UTATMajorLootToolComponent::UTATMajorLootToolComponent()
{
   // Still make component, since the mesh will be set later
   ToolVisuals1P.SkipComponentWithoutMesh = false;
   ToolVisuals3P.SkipComponentWithoutMesh = false;
}

bool UTATMajorLootToolComponent::IsReady() const
{
   return Super::IsReady() && _lootParams.IsValid();
}

UTATLootInventoryComponent* UTATMajorLootToolComponent::GetOwningInventory() const
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

FTATLootInstance UTATMajorLootToolComponent::GetLootInstanceData() const
{
   if (UTATLootInventoryComponent* owningInventory = GetOwningInventory())
   {
      FTATLootInstance lootInstance;
      if (owningInventory->GetMajorLootById(_lootParams.LootInstanceID, lootInstance))
      {
         return lootInstance;
      }
   }

   return FTATLootInstance();
}

void UTATMajorLootToolComponent::OnRegister()
{
   Super::OnRegister();

   if (_HasAuthority() && ensure(_lootParams.IsValid()))
   {
      _OnLootParamsAvailable();
   }
}

void UTATMajorLootToolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(UTATMajorLootToolComponent, _lootParams, COND_InitialOnly);
}

bool UTATMajorLootToolComponent::OnAddToToolSet_Implementation()
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

bool UTATMajorLootToolComponent::OnRemoveFromToolSet_Implementation()
{
   return Super::OnRemoveFromToolSet_Implementation();
}

#if WITH_EDITOR
bool UTATMajorLootToolComponent::CanEditChange(const FProperty* inProperty) const
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

bool UTATMajorLootToolComponent::_HasAuthority() const
{
   if (AActor* owner = GetOwner())
   {
      return owner->HasAuthority();
   }
   return false;
}

void UTATMajorLootToolComponent::_OnRep_LootParams()
{
   if (_lootParams.IsValid())
   {
      _OnLootParamsAvailable();
   }
}

void UTATMajorLootToolComponent::_OnLootParamsAvailable()
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

void UTATMajorLootToolComponent::_InjectIntoInputPrompts(const FTATLootInfo& lootInfo)
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
