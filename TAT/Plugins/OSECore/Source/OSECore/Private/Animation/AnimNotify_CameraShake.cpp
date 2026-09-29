// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/AnimNotify_CameraShake.h"

// ose
#include "OSECommon.h"

// ue4
#include "Camera/CameraShakeBase.h"
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotify_CameraShake)

namespace AnimNotifyCameraShakeUtl
{
   APlayerCameraManager* GetLocalPlayerCameraManager(USkeletalMeshComponent* meshComp)
   {
      if (APlayerController* pc = UOSECommon::GetController<APlayerController>(meshComp->GetOwner()))
      {
         if (pc->IsLocalController())
         {
            return pc->PlayerCameraManager;
         }
      }
      return nullptr;
   }
}

//---------------------------------------------------------------------------------------
// UAnimNotify_CameraShake
//---------------------------------------------------------------------------------------

UAnimNotify_CameraShake::UAnimNotify_CameraShake(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

FString UAnimNotify_CameraShake::GetNotifyName_Implementation() const
{
   return FString::Printf(TEXT("CameraShake: %s"), CameraShakeClass.Get() ? *CameraShakeClass.Get()->GetName() : TEXT("None"));
}

void UAnimNotify_CameraShake::Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference)
{
   Super::Notify(meshComp, animation, eventReference);

   if (APlayerCameraManager* playerCameraMgr = AnimNotifyCameraShakeUtl::GetLocalPlayerCameraManager(meshComp))
   {
      playerCameraMgr->StartCameraShake(CameraShakeClass, Scale, PlaySpace, UserPlaySpaceRot);
   }
}

