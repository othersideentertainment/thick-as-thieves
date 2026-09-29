// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ToolVisuals.h"

#include "Items/ToolHolderInterface.h"
#include "OSECoreCollision.h"
#include "Animation/AnimInstance.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolVisuals)

// Returns the animation montage for the specified anim type
class UAnimMontage* FToolAnimations::GetMontage(EToolAnimation toolAnimation)
{
   switch (toolAnimation)
   {
   case EToolAnimation::Equip: return Equip;
   case EToolAnimation::Unequip: return Unequip;
   case EToolAnimation::Stow: return Stow;
   case EToolAnimation::Unstow: return Unstow;
   case EToolAnimation::EquipUnusable: return EquipUnusable;
   }

   return nullptr;
}

FToolVisuals::FToolVisuals()
   : RelativeLocation(FVector::ZeroVector)
   , RelativeRotation(FRotator::ZeroRotator)
   , Perspective(EMeshPerspective::Default)
   , MeshClass(USkeletalMeshComponent::StaticClass())
   , MeshAsset(nullptr)
   , MeshComponent(nullptr)
{
}

USkeletalMeshComponent* FToolVisuals::CreateMeshComponent(AActor* owner)
{
   check(MeshComponent == nullptr);

   if ((SkipComponentWithoutMesh && MeshAsset == nullptr) || (SkipComponentOnServer && owner->IsNetMode(NM_DedicatedServer)))
   {
      return nullptr;
   }

   UClass* classToUse = MeshClass;
   if (classToUse == nullptr)
      classToUse = USkeletalMeshComponent::StaticClass();

   MeshComponent = NewObject<USkeletalMeshComponent>(owner, classToUse);
   MeshComponent->SetRelativeLocation(RelativeLocation);
   MeshComponent->SetRelativeRotation(RelativeRotation);
   MeshComponent->SetSkeletalMesh(MeshAsset);
   MeshComponent->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::OnlyTickPoseWhenRendered;
   MeshComponent->bUseAttachParentBound = true;

   switch (Perspective)
   {
   case EMeshPerspective::ThirdPerson:
   {
      MeshComponent->SetOwnerNoSee(true);
      MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
      MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
      MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
      MeshComponent->CastShadow = true;
      MeshComponent->bCastHiddenShadow = true;
   }
   break;

   case EMeshPerspective::FirstPerson:
   {
      MeshComponent->SetOnlyOwnerSee(true);
      MeshComponent->SetCollisionObjectType(ECC_WorldDynamic);
      MeshComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
      MeshComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
      MeshComponent->bCastDynamicShadow = false;
      MeshComponent->bReceivesDecals = false;
      MeshComponent->CastShadow = false;
   }
   break;
   }

   HideMeshComponent();

   return MeshComponent;
}

void FToolVisuals::RegisterMeshComponent(AActor* owner)
{
   if (MeshComponent == nullptr)
   {
      return;
   }

   USceneComponent* parentToUse = FindToolRoot(owner, owner->GetRootComponent(), Perspective);

   MeshComponent->SetRelativeLocation(RelativeLocation);
   MeshComponent->SetRelativeRotation(RelativeRotation);

   AttachParent = parentToUse;
   if (MeshComponent->IsVisible())
   {
      MeshComponent->AttachToComponent(parentToUse, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
   }

   if (!MeshComponent->IsRegistered())
      MeshComponent->RegisterComponent();

   if (MeshAnimClass)
   {
      // We have an explicitly defined anim blueprint type to use for the tool mesh
      MeshComponent->SetAnimInstanceClass(MeshAnimClass);
   }
   else
   {
      // If our tool root is a skeletal mesh, and it shares the same skeleton as
      // the tool mesh, then set the tool to use the same animation pose
      // data automatically. This allows us to create skeleton-specific tool meshes
      // that can animate like their parent.
      if (USkeletalMeshComponent* parentSkeletalMeshComponent = Cast<USkeletalMeshComponent>(parentToUse))
      {
         USkinnedAsset* parentSkinnedAsset = parentSkeletalMeshComponent->GetSkinnedAsset();
         USkinnedAsset* toolSkinnedAsset = MeshComponent->GetSkinnedAsset();
         if (parentSkinnedAsset && toolSkinnedAsset && (parentSkinnedAsset->GetSkeleton() == toolSkinnedAsset->GetSkeleton()))
         {
            MeshComponent->SetLeaderPoseComponent(parentSkeletalMeshComponent);
         }
      }
   }
}

void FToolVisuals::UnregisterMeshComponent()
{
   if (MeshComponent == nullptr)
   {
      return;
   }
   
   if (MeshComponent->IsRegistered())
      MeshComponent->UnregisterComponent();
}

void FToolVisuals::DestroyMeshComponent()
{
   // this can occur if the component is destroyed before registering it
   if (MeshComponent)
   {
      MeshComponent->DestroyComponent();
      MeshComponent = nullptr;
   }
}

// Shows the visuals
void FToolVisuals::ShowMeshComponent()
{
   if (MeshComponent == nullptr)
   {
      return;
   }

   if (Perspective == EMeshPerspective::ThirdPerson)
   {
      MeshComponent->CastShadow = true;
      MeshComponent->bCastHiddenShadow = true;
   }

   // Only attach when visible
   if (USceneComponent* parentToUse = AttachParent.Get())
   {
      MeshComponent->AttachToComponent(parentToUse, FAttachmentTransformRules::KeepRelativeTransform, AttachPoint);
   }

   MeshComponent->SetVisibility(true, true);
   MeshComponent->SetComponentTickEnabled(true);
}

// Hides the visuals
void FToolVisuals::HideMeshComponent()
{
   if (MeshComponent == nullptr)
   {
      return;
   }

   if (Perspective == EMeshPerspective::ThirdPerson)
   {
      MeshComponent->CastShadow = false;
      MeshComponent->bCastHiddenShadow = false;
   }

   // detach if not shown
   MeshComponent->DetachFromComponent(FDetachmentTransformRules::KeepRelativeTransform);

   MeshComponent->SetVisibility(false, true);
   MeshComponent->SetComponentTickEnabled(false);
}

// Utility method to find the scene root to attach the visuals to
USceneComponent* FToolVisuals::FindToolRoot(AActor* owner, USceneComponent* parent, EMeshPerspective perspective)
{
   // Starting with the passed in parent, move up the hierarchy to find an tool holder
   USceneComponent* parentToUse = parent;
   while (parentToUse != nullptr && !parentToUse->Implements<UToolHolderInterface>())
   {
      parentToUse = parentToUse->GetAttachParent();
   }

   // If we haven't found one, fall back to the owner's root component
   if (parentToUse == nullptr)
   {
      parentToUse = owner->GetRootComponent();
   }

   // Check if our current parent implements the interface
   IToolHolderInterface* foundHolder = Cast<IToolHolderInterface>(parentToUse);
   if (foundHolder == nullptr)
   {
      // Still no luck finding the interface. Check the owner itself
      foundHolder = Cast<IToolHolderInterface>(owner);
   }

   // Finally, use the interface if we found it
   if (foundHolder != nullptr)
   {
      USceneComponent* candidate = foundHolder->GetToolRoot(perspective);
      if (candidate != nullptr)
         return candidate;
   }

   // No luck finding the interface
   return parent;
}
