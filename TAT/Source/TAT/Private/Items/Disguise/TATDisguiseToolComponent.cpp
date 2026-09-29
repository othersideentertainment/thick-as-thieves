// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/Disguise/TATDisguiseToolComponent.h"

// tat
#include "Disguise/TATDisguiseComponent.h"
#include "Disguise/TATDisguisableCharacterInterface.h"

// ose
#include "Items/ToolSetComponent.h"

// ue
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDisguiseToolComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATDisguiseToolComponent, Log, Log);

void UTATDisguiseToolComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(UTATDisguiseToolComponent, _disguiseToolParams);
}

bool UTATDisguiseToolComponent::OnUnequip_Implementation()
{
   if (ShouldRemoveDisguiseOnUnEquip == false)
      return Super::OnUnequip_Implementation();

   if(GetOwner() != nullptr && GetOwner()->HasAuthority())
   {
      if(UTATDisguiseComponent* disguiseComponent = ITATDisguisableCharacterInterface::Execute_GetDisguiseComponent(GetOwnerCharacter()))
      {
         if(disguiseComponent->IsDisguiseActive())
         {
            disguiseComponent->AuthorityEndDisguise();
         }
      }
   }
   return Super::OnUnequip_Implementation();
}

void UTATDisguiseToolComponent::OnDisguiseParamsUpdated_Implementation(const FTATDisguiseToolParams& newParams)
{
   auto updateMesh = [](FToolVisuals& visuals, USkeletalMesh* mesh)
   {
      if (visuals.MeshComponent)
      {
         visuals.MeshComponent->SetSkeletalMeshAsset(mesh);
      }
      else
      {
         visuals.MeshAsset = mesh;
      }
   };

   // explicitly skipping the unequipped mesh
   updateMesh(ToolVisuals1P, newParams.ToolMesh1P);
   updateMesh(ToolVisuals3P, newParams.ToolMesh3P);
}

void UTATDisguiseToolComponent::AuthoritySetToolDisguiseFromCharacterType(TSubclassOf<ACharacter> disguisedAs)
{
   check(GetOwner() != nullptr && GetOwner()->HasAuthority());
   if (disguisedAs == nullptr)
   {
      return;
   }

   TSubclassOf<UToolComponent> baseTool;

   // Grab the default tools for this character and pick the first valid one we find
   ACharacter* charCDO = disguisedAs->GetDefaultObject<ACharacter>();
   check(charCDO != nullptr);
   if (UToolSetComponent* toolsetCDO = charCDO->FindComponentByClass<UToolSetComponent>())
   {
      const TArray<FOSEDefaultToolEntry>& defaultTools = toolsetCDO->GetDefaultTools();
      for (const FOSEDefaultToolEntry& defaultTool : defaultTools)
      {
         if (defaultTool.ToolClass)
         {
            baseTool = defaultTool.ToolClass;
            break;
         }
      }
   }

   if (baseTool)
   {
      AuthoritySetToolDisguise(baseTool);
   }
}

void UTATDisguiseToolComponent::AuthoritySetToolDisguise(TSubclassOf<UToolComponent> disguisedAsTool)
{
   check(GetOwner() != nullptr && GetOwner()->HasAuthority());
   if (disguisedAsTool == nullptr)
   {
      GetOwner()->FlushNetDormancy();
      _disguiseToolParams = FTATDisguiseToolParams{};
      _OnRep_DisguiseToolParams();
   }
   else
   {
      UToolComponent* toolCDO = disguisedAsTool.GetDefaultObject();
      check(toolCDO != nullptr);
      FTATDisguiseToolParams newParams;
      newParams.ToolMesh1P = toolCDO->GetToolSkeletalMeshAssetFirstPerson();
      newParams.ToolMesh3P = toolCDO->GetToolSkeletalMeshAssetThirdPerson();
      GetOwner()->FlushNetDormancy();
      _disguiseToolParams = newParams;
      _OnRep_DisguiseToolParams();
   }
}

void UTATDisguiseToolComponent::AuthorityRevertToolDisguise()
{
   check(GetOwner() != nullptr && GetOwner()->HasAuthority());
   UTATDisguiseToolComponent* selfCDO = GetClass()->GetDefaultObject<UTATDisguiseToolComponent>();
   check(selfCDO != nullptr);
   FTATDisguiseToolParams newParams;
   newParams.ToolMesh1P = selfCDO->ToolVisuals1P.MeshAsset;
   newParams.ToolMesh3P = selfCDO->ToolVisuals3P.MeshAsset;
   GetOwner()->FlushNetDormancy();
   _disguiseToolParams = newParams;
   _OnRep_DisguiseToolParams();
}

void UTATDisguiseToolComponent::_OnRep_DisguiseToolParams()
{
   OnDisguiseParamsUpdated(_disguiseToolParams);
}
