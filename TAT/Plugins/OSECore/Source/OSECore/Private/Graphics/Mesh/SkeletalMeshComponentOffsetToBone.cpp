// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2019 OtherSide Entertainment, Inc. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "Graphics/Mesh/SkeletalMeshComponentOffsetToBone.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SkeletalMeshComponentOffsetToBone)

FMatrix USkeletalMeshComponentOffsetToBone::GetRenderMatrix() const
{
   if (RenderRelativeToSocket != NAME_None)
   {
      // account for offset
      const FMatrix LocalToWorld = Super::GetRenderMatrix();
      FTransform socketTransform = Super::GetSocketTransform(RenderRelativeToSocket, RTS_Component);
      socketTransform.ScaleTranslation(-1);
      socketTransform.SetRotation(FQuat::Identity);
      return socketTransform.ToMatrixNoScale() * LocalToWorld;
   }

   return Super::GetRenderMatrix();
}

// NOTE: copied mostly verbatim to override the Transform parameter passed to CalcMeshBound
//       As such, the style has been left alone so as to make the changes more isolated in case future
//       updates are needed.
FBoxSphereBounds USkeletalMeshComponentOffsetToBone::CalcBounds(const FTransform& LocalToWorld) const
{
   //SCOPE_CYCLE_COUNTER(STAT_CalcSkelMeshBounds);

   // fixme laurent - extend concept of LocalBounds to all SceneComponent
   // as rendered calls CalcBounds*() directly in FScene::UpdatePrimitiveTransform, which is pretty expensive for SkelMeshes.
   // No need to calculated that again, just use cached local bounds.
   if (bCachedWorldSpaceBoundsUpToDate || bCachedLocalBoundsUpToDate)
   {
      FBoxSphereBounds Result;
      if (bCachedLocalBoundsUpToDate)
      {
         Result = CachedWorldOrLocalSpaceBounds.TransformBy(LocalToWorld);
      }
      else
      {
         Result = CachedWorldOrLocalSpaceBounds.TransformBy(CachedWorldToLocalTransform * LocalToWorld.ToMatrixWithScale());
      }

      if (bIncludeComponentLocationIntoBounds)
      {
         const FVector ComponentLocation = GetComponentLocation();
         return Result + FBoxSphereBounds(ComponentLocation, FVector(1.0f), 1.0f);
      }
      else
      {
         return Result;
      }
   }
   // Calculate new bounds
   else
   {
		FVector RootBoneOffset = RootBoneTranslation;

		// if to use MasterPoseComponent's fixed skel bounds, 
		// send MasterPoseComponent's Root Bone Translation
		if (LeaderPoseComponent.IsValid())
		{
			const USkinnedMeshComponent* const LeaderPoseComponentInst = LeaderPoseComponent.Get();
			check(LeaderPoseComponentInst);
			if (LeaderPoseComponentInst->GetSkinnedAsset() &&
            LeaderPoseComponentInst->bComponentUseFixedSkelBounds &&
            LeaderPoseComponentInst->IsA((USkeletalMeshComponent::StaticClass())))
			{
				const USkeletalMeshComponent* BaseComponent = CastChecked<USkeletalMeshComponent>(LeaderPoseComponentInst);
				RootBoneOffset = BaseComponent->RootBoneTranslation; // Adjust bounds by root bone translation
			}
		}

      const bool bCacheLocalSpaceBounds = /*CVarCacheLocalSpaceBounds.GetValueOnGameThread()*/1 != 0;
      const FTransform CachedBoundsTransform = bCacheLocalSpaceBounds ? FTransform::Identity : LocalToWorld;

      // Change: offset bounds by location of socket
      FTransform meshBoundsTransform = CachedBoundsTransform;
      if (RenderRelativeToSocket != NAME_None)
      {
         FTransform socketTransform = Super::GetSocketTransform(RenderRelativeToSocket, RTS_Component);
         socketTransform.ScaleTranslation(-1);
         socketTransform.SetRotation(FQuat::Identity);
         FTransform::Multiply(&meshBoundsTransform, &socketTransform, &CachedBoundsTransform);
      }
      FBoxSphereBounds NewBounds = CalcMeshBound((FVector3f)RootBoneOffset, bHasValidBodies, meshBoundsTransform);

      if (bIncludeComponentLocationIntoBounds)
      {
         const FVector ComponentLocation = GetComponentLocation();
         NewBounds = NewBounds + FBoxSphereBounds(ComponentLocation, FVector(1.0f), 1.0f);
      }

      AddClothingBounds(NewBounds, LocalToWorld);

      bCachedLocalBoundsUpToDate = true;
      CachedWorldOrLocalSpaceBounds = NewBounds;
      bCachedLocalBoundsUpToDate = bCacheLocalSpaceBounds;
      bCachedWorldSpaceBoundsUpToDate = !bCacheLocalSpaceBounds;

      if (bCacheLocalSpaceBounds)
      {
         CachedWorldToLocalTransform.SetIdentity();
         return NewBounds.TransformBy(LocalToWorld);
      }
      else
      {
         CachedWorldToLocalTransform = LocalToWorld.ToInverseMatrixWithScale();
         return NewBounds;
      }
   }
}

// NOTE: copied mostly verbatim to override the Transform parameter passed to GetBoneTransform
//       As such, the style has been left alone so as to make the changes more isolated in case future
//       updates are needed.
FTransform USkeletalMeshComponentOffsetToBone::GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace) const
{
   // Based off of USkinnedMeshComponent::GetSocketTransform
   QUICK_SCOPE_CYCLE_COUNTER(USkeletalMeshComponentOffsetToBone_GetSocketTransform);

   FTransform OutSocketTransform = GetComponentTransform();

   if (InSocketName != NAME_None)
   {
      // Change
      FTransform localToWorld = GetComponentTransform();
      if (RenderRelativeToSocket != NAME_None && TransformSpace != RTS_ParentBoneSpace)
      {
         FTransform socketTransform = Super::GetSocketTransform(RenderRelativeToSocket, RTS_Component);
         socketTransform.ScaleTranslation(-1);
         socketTransform.SetRotation(FQuat::Identity);
         FTransform::Multiply(&localToWorld, &socketTransform, &localToWorld);
      }

      int32 SocketBoneIndex;
      FTransform SocketLocalTransform;
      USkeletalMeshSocket const* const Socket = GetSocketInfoByName(InSocketName, SocketLocalTransform, SocketBoneIndex);
      // apply the socket transform first if we find a matching socket
      if (Socket)
      {
         if (TransformSpace == RTS_ParentBoneSpace)
         {
            //we are done just return now
            return SocketLocalTransform;
         }

         if (SocketBoneIndex != INDEX_NONE)
         {
            FTransform BoneTransform = GetBoneTransform(SocketBoneIndex, localToWorld); //< Changed
            OutSocketTransform = SocketLocalTransform * BoneTransform;
         }
      }
      else
      {
         int32 BoneIndex = GetBoneIndex(InSocketName);
         if (BoneIndex != INDEX_NONE)
         {
            OutSocketTransform = GetBoneTransform(BoneIndex, localToWorld); //< Changed

            if (TransformSpace == RTS_ParentBoneSpace)
            {
               FName ParentBone = GetParentBone(InSocketName);
               int32 ParentIndex = GetBoneIndex(ParentBone);
               if (ParentIndex != INDEX_NONE)
               {
                  return OutSocketTransform.GetRelativeTransform(GetBoneTransform(ParentIndex));
               }
               return OutSocketTransform.GetRelativeTransform(GetComponentTransform());
            }
         }
      }
   }

   switch (TransformSpace)
   {
   case RTS_Actor:
   {
      if (AActor* Actor = GetOwner())
      {
         return OutSocketTransform.GetRelativeTransform(Actor->GetTransform());
      }
      break;
   }
   case RTS_Component:
   {
      return OutSocketTransform.GetRelativeTransform(GetComponentTransform());
   }
   }

   return OutSocketTransform;
}


