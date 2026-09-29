// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkeletalMeshComponent1P.generated.h"


//--------------------------------------------------------------------------------------------------
/// First person mesh options
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FSkeletalMesh1PSettings
{
   GENERATED_BODY()

public:

   /// The field of view to render the mesh regardless of the actual camera field of view.
   /// This value should match the settings used to author first person animations.
   UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Mesh")
   float MeshFOV = 85.0f;

   /// The aspect ratio to render the mesh regardless of the actual camera aspect ratio.
   /// This value should match the settings used to author first person animations.
   UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Mesh")
   float MeshAspectRatio = 1.777778f; // 16:9
};


//--------------------------------------------------------------------------------------------------
/// Skeletal mesh component that can be used as a replacement for first person mesh rendering.
/// This version will modify the rendering of the mesh to maintain a specific field of view.
//--------------------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API USkeletalMeshComponent1P : public USkeletalMeshComponent
{
   GENERATED_BODY()

public:

   USkeletalMeshComponent1P();

   /// When rendered as part of a local player view, this returns a local to world transform such
   /// that the current view settings will end up transforming it to the desired view settings.
   virtual FMatrix GetRenderMatrix() const override;

   /// Modifies the bounds to include the attached parent in addition to the bounds for this mesh
   virtual void UpdateBounds() override;

   /// Calculate socket transformations taking the current view settings into account so world space transforms look correct.
   UFUNCTION(BlueprintCallable)
   FTransform GetCorrectedWorldSocketTransform(FName socketName) const;

protected:

   /// Allows us to use first person settings from another source, such as our parent mesh.
   virtual bool QueryFirstPersonSettings(FSkeletalMesh1PSettings& outSettings) const;

   // @TODO: Allow these settings to be driven from DCC animated properties, and update the
   //        values automatically based on the current playing animation instance?

   /// Merges the default mesh bounds with the attach parent bounds.
   /// Only takes effect when bUseAttachParentBound is off.
   UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Rendering|FirstPerson")
   bool MergeWithAttachParentBounds = true;

   /// Enables first person mesh modifications
   UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Mesh|FirstPerson")
   bool FirstPersonEnabled = true;

   /// First person mesh settings
   UPROPERTY(Interp, EditAnywhere, BlueprintReadWrite, Category = "Mesh|FirstPerson", meta = (editcondition = "FirstPersonEnabled"))
   FSkeletalMesh1PSettings FirstPersonSettings;
};
