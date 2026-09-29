// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Animation/AnimNotifyState_OSEToolVisualState.h"

// ue4
#include "Components/SkeletalMeshComponent.h"

// ose
#include "Items/ToolSetSystemInterface.h"
#include "Items/ToolSetInterface.h"
#include "Items/ToolComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_OSEToolVisualState)


void UAnimNotifyState_OSEToolVisualState::NotifyBegin(class USkeletalMeshComponent* meshComp, class UAnimSequenceBase* animation, float totalDuration, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyBegin(meshComp, animation, totalDuration, eventReference);

   if (!VisualStateTag.IsValid())
      return;

   AActor* owningActor = meshComp->GetOwner();

   if (auto toolSetSystemInterface = Cast<IToolSetSystemInterface>(owningActor))
   {
      if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystemInterface->GetToolSetInterface())
      {
         if (UToolComponent* tool = toolSetInterface->GetCurrentTool())
         {
            IToolInterface::Execute_OnVisualStateBegin(tool, VisualStateTag, totalDuration);
         }
      }
   }
   
}

void UAnimNotifyState_OSEToolVisualState::NotifyEnd(class USkeletalMeshComponent* meshComp, class UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyEnd(meshComp, animation, eventReference);

   if (!VisualStateTag.IsValid())
      return;

   AActor* owningActor = meshComp->GetOwner();

   if (auto toolSetSystemInterface = Cast<IToolSetSystemInterface>(owningActor))
   {
      if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystemInterface->GetToolSetInterface())
      {
         if (UToolComponent* tool = toolSetInterface->GetCurrentTool())
         {
            IToolInterface::Execute_OnVisualStateEnd(tool, VisualStateTag);
         }
      }
   }
}

