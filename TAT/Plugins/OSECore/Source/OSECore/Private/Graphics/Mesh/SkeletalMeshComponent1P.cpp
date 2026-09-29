// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// OSE
#include "Graphics/Mesh/SkeletalMeshComponent1P.h"
#include "Camera/OSECameraUtils.h"
#include "Camera/OSECameraSettings.h"

// UE4
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SkeletalMeshComponent1P)


namespace FirstPersonUtils
{
   // @TODO seems like something that would exist
   static const APlayerController* FindPlayerController(const AActor* InActor)
   {
      const AActor* TestActor = InActor;
      while (TestActor)
      {
         if (const APlayerController* CastController = Cast<APlayerController>(TestActor))
            return CastController;

         if (const APawn* Pawn = Cast<APawn>(TestActor))
            return Cast<APlayerController>(Pawn->GetController());

         TestActor = TestActor->GetOwner();
      }

      return nullptr;
   }

   // Returns the minimal view info for the given actor by searching for a player controller
   static FMinimalViewInfo ComputeCurrentViewInfo(const AActor* InActor)
   {
      FMinimalViewInfo ViewInfo;

      if (InActor != nullptr)
      {
         // Walk up the actor owner hierarchy, searching for a player controller
         const APlayerController* PlayerController = FirstPersonUtils::FindPlayerController(InActor);
         if (PlayerController != nullptr)
         {
            if (PlayerController->PlayerCameraManager != nullptr)
            {
               // This will give us the most accurate view info
               ViewInfo = PlayerController->PlayerCameraManager->GetCameraCacheView();
               ViewInfo.FOV = PlayerController->PlayerCameraManager->GetFOVAngle();
            }

            // Use the controllers view point
            PlayerController->GetPlayerViewPoint(/*out*/ ViewInfo.Location, /*out*/ ViewInfo.Rotation);
         }
         else
         {
            // No player controller, use the actor transform
            ViewInfo.Location = InActor->GetActorLocation();
            ViewInfo.Rotation = InActor->GetActorRotation();
         }
      }

      // Probably not necessary, but make sure the desired FOV is the same
      ViewInfo.DesiredFOV = ViewInfo.FOV;
      return ViewInfo;
   }

   // Returns a delta matrix to apply to the render matrix. The resulting matrix, when it has the
   // render view projection matrix applied to it, will return our modified view projection matrix
   // instead. If both were the same, the delta matrix would be the identity matrix
   static FMatrix ComputeFirstPersonDelta(const FSkeletalMesh1PSettings& inSettings, const AActor* inActor)
   {
      const FMinimalViewInfo currentViewInfo = FirstPersonUtils::ComputeCurrentViewInfo(inActor);
      const FTransform viewToWorldTransform = FTransform(currentViewInfo.Rotation, currentViewInfo.Location, FVector::One());

      const FMatrix cameraToWorld = viewToWorldTransform.ToMatrixWithScale();
      const FMatrix worldToCamera = cameraToWorld.Inverse();

      const float tanHalfMESH = FMath::Tan(FMath::DegreesToRadians(inSettings.MeshFOV / 2));
      const float tanHalfGAME = FMath::Tan(FMath::DegreesToRadians(currentViewInfo.FOV / 2));

      const float scaleFactor = (tanHalfMESH / tanHalfGAME) * 0.5f + 0.5f;

      const FMatrix scaleMatrix = FMatrix(
            FPlane(1, 0, 0, 0),
            FPlane(0, 1 / scaleFactor, 0, 0),
            FPlane(0, 0, 1 / scaleFactor, 0),
            FPlane(0, 0, 0, 1));

      return worldToCamera * scaleMatrix * cameraToWorld;
   }
}

USkeletalMeshComponent1P::USkeletalMeshComponent1P()
   : Super()
{
   // Make sure our CDO defaults to the default project settings values
   const FOSECameraParams& playerParams = UOSECameraSettings::Get().GetPlayerParams();
   FirstPersonSettings.MeshFOV = playerParams.FieldOfView;
   FirstPersonSettings.MeshAspectRatio = playerParams.AspectRatio;
   
   FirstPersonPrimitiveType = EFirstPersonPrimitiveType::FirstPerson;
}


// When rendered as part of a local player view, this returns a local to world transform such
// that the current view settings will end up transforming it to the desired view settings.
FMatrix USkeletalMeshComponent1P::GetRenderMatrix() const
{
   FSkeletalMesh1PSettings localSettings;
   const bool localEnabled = QueryFirstPersonSettings(localSettings);

   if (localEnabled)
   {
      const FMatrix localToWorld = Super::GetRenderMatrix();
      const FMatrix deltaMatrix = FirstPersonUtils::ComputeFirstPersonDelta(localSettings, GetOwner());
      return localToWorld * deltaMatrix;
   }

   return Super::GetRenderMatrix();
}

// Modifies the bounds to include the attached parent in addition to the bounds for this mesh
void USkeletalMeshComponent1P::UpdateBounds()
{
   Super::UpdateBounds();

   if (MergeWithAttachParentBounds && !bUseAttachParentBound)
   {
      if (const USceneComponent* attachParent = GetAttachParent())
      {
         Bounds = Bounds + attachParent->Bounds;
      }
   }
}

// Allows us to use first person settings from another source, such as our parent mesh.
bool USkeletalMeshComponent1P::QueryFirstPersonSettings(FSkeletalMesh1PSettings& outSettings) const
{
   bool outEnabled = FirstPersonEnabled;
   outSettings = FirstPersonSettings;

   // If our parent mesh is also a 1P mesh, then use its settings
   if (GetAttachParent() && GetAttachParent()->IsA(USkeletalMeshComponent1P::StaticClass()))
   {
      const auto parent1P = CastChecked<USkeletalMeshComponent1P>(GetAttachParent());
      outEnabled = parent1P->FirstPersonEnabled;
      outSettings = parent1P->FirstPersonSettings;
   }

   return outEnabled;
}

FTransform USkeletalMeshComponent1P::GetCorrectedWorldSocketTransform(FName socketName) const
{
   FSkeletalMesh1PSettings localSettings;
   const bool localEnabled = QueryFirstPersonSettings(localSettings);

   if (localEnabled)
   {
      //If we're transforming into world space we need to take the first person FOV transforms into account.
      const FMatrix delta = FirstPersonUtils::ComputeFirstPersonDelta(localSettings, GetOwner());
      FTransform superTransform = GetSocketTransform(socketName, ERelativeTransformSpace::RTS_World);
      
      // Concat the two transforms in matrices instead of FTransforms because when FTransform converts a matrix it will lose projection information.
      FTransform resultTransform;
      resultTransform.SetFromMatrix(superTransform.ToMatrixWithScale() * delta);
      return resultTransform;
   }

   return GetSocketTransform(socketName, ERelativeTransformSpace::RTS_World);
}
