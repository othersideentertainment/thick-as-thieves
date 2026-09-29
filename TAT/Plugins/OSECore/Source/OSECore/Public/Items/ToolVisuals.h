// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"

#include "ToolVisuals.generated.h"

class USkeletalMesh;
class USkeletalMeshComponent;

//--------------------------------------------------------------------------------------------------
/// Specifies how a mesh will be configured based on perspective (i.e., first or third person)
//--------------------------------------------------------------------------------------------------
UENUM(BlueprintType)
enum class EMeshPerspective : uint8
{
   /// Mesh is configured for rendering in third person perspective
   ThirdPerson,

   /// Mesh is configured for rendering in first person perspective
   FirstPerson,

   Default = ThirdPerson UMETA(Hidden)
};


//--------------------------------------------------------------------------------------------------
/// Specifies the types of supported tool animations
//--------------------------------------------------------------------------------------------------
UENUM()
enum class EToolAnimation : uint8
{
   /// Play when equipped from tool set
   Equip,

   /// Play when unequipped from tool set
   Unequip,

   /// Play when stowed
   Stow,

   /// Play when unstowed
   Unstow,

   /// Play when equipping a tool that is unusable (i.e. it has no ammo)
   EquipUnusable,

   Default = Equip UMETA(Hidden)
};


//--------------------------------------------------------------------------------------------------
/// Structure defining the animations for a tool
//--------------------------------------------------------------------------------------------------
USTRUCT(BlueprintType)
struct OSECORE_API FToolAnimations
{
   GENERATED_BODY()

public:

   /// Returns the animation montage for the specified anim type
   class UAnimMontage* GetMontage(EToolAnimation toolAnimation);

   /// The animation to play on the tool holder when equipped
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   class UAnimMontage* Equip = nullptr;

   /// The animation to play on the tool holder when unequipped
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   class UAnimMontage* Unequip = nullptr;

   /// The animation to play on the tool holder when stowed during mantle, slide, carry, etc
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   class UAnimMontage* Stow = nullptr;

   /// The animation to play on the tool holder when stowed during mantle, slide, carry, etc
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   class UAnimMontage* Unstow = nullptr;

   /// The animation to play on the tool holder when equipping a tool that is unusable (i.e. it has no ammo)
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   class UAnimMontage* EquipUnusable = nullptr;

   /// Default playback rate for equip/unequip
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   float EquipPlaybackRate = 1.0f;

   /// Stow playback rate for equip/unequip
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   float StowPlaybackRate = 2.0f;

   /// Optional animation layer to apply while equipped
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation, meta = (MustImplement = "/Script/Engine.AnimLayerInterface"))
   TSubclassOf<class UAnimInstance> AnimClassLayer = nullptr;

   /// Whether the override the delay before the tool is complete
   UPROPERTY(EditDefaultsOnly, Category = Animation)
   bool ShouldOverrideEquipCompleteDelay = false;

   // Seconds to wait before tool ability is activated
   UPROPERTY(EditDefaultsOnly, Category = Animation, meta= (EditCondition = "ShouldOverrideEquipCompleteDelay", EditConditionHides))
   float EquipCompleteDelayOverride = 0;
};


//--------------------------------------------------------------------------------------------------
/// Structure defining the data used to represent dynamically created tool visuals
//--------------------------------------------------------------------------------------------------

USTRUCT(BlueprintType)
struct OSECORE_API FToolVisuals
{
   GENERATED_BODY()

public:

   FToolVisuals();

   // Component events
   USkeletalMeshComponent* CreateMeshComponent(AActor* owner);
   void RegisterMeshComponent(AActor* owner);
   void UnregisterMeshComponent();
   void DestroyMeshComponent();

   /// Shows the visuals
   void ShowMeshComponent();

   /// Hides the visuals
   void HideMeshComponent();

   /// Location of the component relative to its parent
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Transform)
   FVector RelativeLocation;

   /// Rotation of the component relative to its parent
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Transform)
   FRotator RelativeRotation;

   /// Attach point on the parent mesh
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Transform)
   FName AttachPoint;

   /// Perspective to render the mesh
   UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Mesh)
   EMeshPerspective Perspective;

   // Skip creating a component if there is no mesh asset configured
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Mesh)
   bool SkipComponentWithoutMesh = true;

   // Skip creating a component on dedicated server
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Mesh)
   bool SkipComponentOnServer = true;

   /// Optional mesh class
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, NoClear, Category = Mesh)
   TSubclassOf<USkeletalMeshComponent> MeshClass;

   /// The skeletal mesh used by this tool
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Mesh)
   USkeletalMesh* MeshAsset;

   /// Optional animation blueprint to apply to the tool's skeletal mesh while equipped
   /// Note that you likely don't need this if the tool's skeletal mesh shares a skeleton with the tool holder
   UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = Animation)
   TSubclassOf<class UAnimInstance> MeshAnimClass = nullptr;

   /// Generated tool mesh component
   UPROPERTY(Transient, BlueprintReadOnly, Category = Mesh)
   USkeletalMeshComponent* MeshComponent;

   /// runtime attach parent
   TWeakObjectPtr<USceneComponent> AttachParent;

   /// Utility method to find the scene root to attach the visuals to
   static USceneComponent* FindToolRoot(AActor* owner, USceneComponent* parent, EMeshPerspective perspective);
};

// Structure that contains the name of sockets to attach to for first person and third person
// TODO: Move to different file?
USTRUCT(BlueprintType)
struct OSECORE_API FPerspectiveAttachPoints
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
   FName FirstPersonSocket;

   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
   FName ThirdPersonSocket;
};
