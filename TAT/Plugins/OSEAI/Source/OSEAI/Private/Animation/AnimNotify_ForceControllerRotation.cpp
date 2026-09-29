// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Animation/AnimNotify_ForceControllerRotation.h"

#include "AI/OSEAIController.h"

AOSEAIController* GetAIControllerFromMesh(const USkeletalMeshComponent* meshComponent)
{
   if(const APawn* pawn = Cast<APawn>(meshComponent->GetOwner()))
   {
      return Cast<AOSEAIController>(pawn->GetController());
   }
   return nullptr;
}

void UAnimNotify_ForceControllerRotation::NotifyBegin(USkeletalMeshComponent* meshComp,
   UAnimSequenceBase* animation,
   const float totalDuration,
   const FAnimNotifyEventReference& eventReference)
{
   if(AOSEAIController* aiController = GetAIControllerFromMesh(meshComp))
   {
      aiController->ClearForcedControlRotation();
   }
   Super::NotifyBegin(meshComp, animation, totalDuration, eventReference);
}

void UAnimNotify_ForceControllerRotation::NotifyTick(USkeletalMeshComponent* meshComp,
   UAnimSequenceBase* animation,
   const float frameDeltaTime,
   const FAnimNotifyEventReference& eventReference)
{
   if(AOSEAIController* aiController = GetAIControllerFromMesh(meshComp))
   {
      const USkeletalMesh* skeletalMesh = meshComp->GetSkeletalMeshAsset();
      if(skeletalMesh == nullptr)
         return;
      const USkeleton* skeleton = skeletalMesh->GetSkeleton();
      if(skeleton == nullptr)
         return;
      const UAnimMontage* montage = Cast<UAnimMontage>(animation);
      if(montage == nullptr)
         return;
      const UAnimInstance* animInstance = meshComp->GetAnimInstance();
      if(animInstance == nullptr)
         return;
      const float montagePosition = animInstance->Montage_GetPosition(montage);
      if(animation->HasCurveData(_lookAroundPitchCurveName) && animation->HasCurveData(_lookAroundYawCurveName))
      {
         FRotator rotator;
         rotator.Yaw = animation->EvaluateCurveData(_lookAroundYawCurveName, montagePosition, false);
         rotator.Pitch = animation->EvaluateCurveData(_lookAroundPitchCurveName, montagePosition, false);
         aiController->SetForcedControlRotation(rotator);
      }
   }
   Super::NotifyTick(meshComp, animation, frameDeltaTime, eventReference);
}

void UAnimNotify_ForceControllerRotation::NotifyEnd(USkeletalMeshComponent* meshComp,
   UAnimSequenceBase* animation,
   const FAnimNotifyEventReference& eventReference)
{
   if(AOSEAIController* aiController = GetAIControllerFromMesh(meshComp))
   {
      aiController->ClearForcedControlRotation();
   }
   Super::NotifyEnd(meshComp, animation, eventReference);
}
