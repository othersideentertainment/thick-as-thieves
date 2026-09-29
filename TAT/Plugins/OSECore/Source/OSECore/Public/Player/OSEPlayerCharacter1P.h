// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

#include "Player/OSEPlayerCharacter.h"
#include "Camera/PlayerCameraAnim.h"

#include "OSEPlayerCharacter1P.generated.h"


//--------------------------------------------------------------------------------------------------
/// A character suitable for use as a 1st person player
//--------------------------------------------------------------------------------------------------

UCLASS()
class OSECORE_API AOSEPlayerCharacter1P
   : public AOSEPlayerCharacter
{
   GENERATED_BODY()
   
public:

   /// Sets default values for this character's properties
   AOSEPlayerCharacter1P(const FObjectInitializer& objectInitializer = FObjectInitializer::Get());

   /// Overridable native event for when play begins for this actor
   virtual void BeginPlay() override;

   /// Overridable function called whenever this actor is being removed from a level
   virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

   /// Overriding to disable shadows on any meshes that are marked as "cast hidden shadow" when the player owner is hidden
   virtual void SetActorHiddenInGame(bool newHidden) override;

   /// Returns third person view mesh data
   FORCEINLINE class USkeletalMeshComponent* GetMesh3P_Head() const { return Mesh3P_Head; }
   FORCEINLINE class USkeletalMeshComponent* GetMesh3P() const { return GetMesh(); }

   /// Returns first person view mesh data
   FORCEINLINE class USkeletalMeshComponent* GetMesh1P_UpperBody() const { return Mesh1P_UpperBody; }
   FORCEINLINE class USkeletalMeshComponent* GetMesh1P_LowerBody() const { return Mesh1P_LowerBody; }
   FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return GetMesh1P_UpperBody(); }

public:
   
   /// Overriding to modify the camera for first person support
   virtual void CalcCamera(float deltaTime, struct FMinimalViewInfo& outResult) override;

private:
   virtual void _ModifyCameraTransform(float deltaTime, FTransform& transform) {}
   virtual void _ModifyFirstPersonMeshViewTransform(float deltaTime, FTransform& transform) {}

private:

   /// Utility method to update the first person mesh to match the camera
   bool UpdateFirstPersonMesh(float deltaTime, const FTransform& CameraXfm);

   /// Configure meshes and settings based on perspective
   static void ConfigureMeshComponent(EMeshPerspective meshPerspective, class USkeletalMeshComponent* meshComponent);

protected:

   /// IToolHolderInterface
   virtual USceneComponent* GetToolRoot(EMeshPerspective meshPerspective) const override;

protected:

   /// Names for mesh components, if you want to use a different class (with ObjectInitializer.SetDefaultSubobjectClass)
   static const FName ComponentName_Mesh1P_UpperBody;
   static const FName ComponentName_Mesh1P_LowerBody;
   static const FName ComponentName_Mesh3P_Head;


   /// Socket on the character mesh to use for first person mesh positioning / orientation
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera)
   FPlayerCameraSocket CameraSocket;

   /// Socket on the character mesh to use for game camera positioning / orientation
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Camera)
   FPlayerCameraSocket GameCameraSocket;

   /// First person view upper body mesh (arms; seen only by self)
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
   class USkeletalMeshComponent* Mesh1P_UpperBody;

   /// First person view lower body mesh (legs; seen only by self)
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
   class USkeletalMeshComponent* Mesh1P_LowerBody;

   /// Third person view head mesh
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
   class USkeletalMeshComponent* Mesh3P_Head;

   bool _didCreate1PMesh { true };

private:
   TArray<TWeakObjectPtr<UMeshComponent>> _shadowCastingMeshComponents;
};
