// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue5
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "TATDisguiseTargetInterface.generated.h"

class UTATCharacterAnimationMappingAsset;
class UAnimMontage;

UENUM(BlueprintType)
enum class ETATDisguiseMeshType : uint8
{
   Skeleton,
   ThirdPersonBody,
   ThirdPersonHead,
   FirstPersonLowerBody,
   FirstPersonUpperBody,
   MAX
};

USTRUCT(BlueprintType)
struct TAT_API FTATDisguiseMovementParams
{
   GENERATED_BODY()

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Movement Params")
   float MaxWalkSpeed = 0.0f;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Disguise Movement Params")
   float MaxAcceleration = 0.0f;

   bool IsValid() const { return MaxWalkSpeed > 0 && MaxAcceleration > 0; }
};

UENUM(BlueprintType)
enum class ETATDisguiseTargetType : uint8
{
   None = 0,
   Civilian = 1,
   Guard = 2
};

/// Interface for characters whose characteristics (like appearance and movement speed) can be copied by the disguise ability.
UINTERFACE(BlueprintType, MinimalAPI, Category = "Disguise", meta = (CannotImplementInterfaceInBlueprint))
class UTATDisguiseTargetInterface : public UInterface
{
   GENERATED_BODY()
};

class TAT_API ITATDisguiseTargetInterface
{
   GENERATED_BODY()

public:
   /// Helper function for implementations of GetDisguiseTargetMesh to make it simple to get a skeletal mesh asset and materials used
   static USkeletalMesh* GetSkeletalMeshAndMaterials(USkeletalMesh* meshAsset, TArray<UMaterialInterface*>& outOverrideMaterials);

   /// Helper function for implementations of GetDisguiseTargetMesh to make it simple to get a skeletal mesh asset and materials used
   static USkeletalMesh* GetSkeletalMeshAndMaterials(USkeletalMeshComponent* meshComponent, TArray<UMaterialInterface*>& outOverrideMaterials);

   /// Gets the mesh and materials a character disguised as this one should use
   UFUNCTION(BlueprintCallable, Category = "Disguise Target Interface")
   virtual USkeletalMesh* GetDisguiseTargetMesh(ETATDisguiseMeshType meshType, TArray<UMaterialInterface*>& outOverrideMaterials) const = 0;

   /// Gets the anim instance class to use for the disguised character in third person
   UFUNCTION(BlueprintCallable, Category = "Disguise Target Interface")
   virtual TSubclassOf<UAnimInstance> GetDisguiseTargetThirdPersonAnimClass() const { return nullptr; }

   /// Gets the anim set layer to use when disguised as this character
   UFUNCTION(BlueprintCallable, Category = "Disguise Target Interface")
   virtual TSubclassOf<UAnimInstance> GetDisguiseTargetAnimSetLayer() const { return nullptr; }
   
   /// Gets this character's animation tag to montage map.
   UFUNCTION(BlueprintCallable, Category = "Disguise Target Interface")
   virtual UTATCharacterAnimationMappingAsset* GetDisguiseTargetCharacterAnimationMapping() const { return nullptr; }

   /// Gets the movement speed of the target so the character being disguised can move at the same speed.
   UFUNCTION(BlueprintCallable, Category = "Disguise Target Interface")
   virtual FTATDisguiseMovementParams GetDisguiseTargetMovementParams() const = 0;

   /// Gets the type of disguise target so the character knows what kind of emotes it can perform
   UFUNCTION(BlueprintCallable, Category = "Disguise Target Interface")
   virtual ETATDisguiseTargetType GetDisguiseTargetType() const = 0;
};
