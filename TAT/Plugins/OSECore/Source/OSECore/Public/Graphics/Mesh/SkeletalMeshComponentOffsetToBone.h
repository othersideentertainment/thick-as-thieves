/*=============================================================================
   UnSkeletalComponent.cpp: Actor component implementation.
=============================================================================*/
// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2019 OtherSide Entertainment, Inc. All rights reserved.
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT

#pragma once

#include "CoreMinimal.h"
#include "Components/SkeletalMeshComponent.h"
#include "SkeletalMeshComponentOffsetToBone.generated.h"

// A skeletal mesh component that is rendered such that it is is rooted as a specific bone
UCLASS(BlueprintType, ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class OSECORE_API USkeletalMeshComponentOffsetToBone : public USkeletalMeshComponent
{
   GENERATED_BODY()

public:
   virtual FMatrix GetRenderMatrix() const override;
   virtual FBoxSphereBounds CalcBounds(const FTransform& localToWorld) const override;
   virtual FTransform GetSocketTransform(FName InSocketName, ERelativeTransformSpace TransformSpace = RTS_World) const override;

protected:

   /// Socket to render relative to
   UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category = "Mesh")
   FName RenderRelativeToSocket;
};
