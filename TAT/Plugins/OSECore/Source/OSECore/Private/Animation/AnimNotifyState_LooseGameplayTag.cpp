// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/AnimNotifyState_LooseGameplayTag.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Character/OSECharacterBase.h"

// ue4
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_LooseGameplayTag)

UAnimNotifyState_LooseGameplayTag::UAnimNotifyState_LooseGameplayTag(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

FString UAnimNotifyState_LooseGameplayTag::GetNotifyName_Implementation() const
{
   FString notifyName = Super::GetNotifyName_Implementation();
   if (GameplayTag.IsValid())
   {
      notifyName = FString::Printf(TEXT("%s: %s"), *notifyName, *GameplayTag.ToString());
   }
   else
   {
      notifyName = FString::Printf(TEXT("%s: NONE"), *notifyName);
   }
   return notifyName;
}

void UAnimNotifyState_LooseGameplayTag::NotifyBegin(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float totalDuration, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyBegin(meshComp, animation, totalDuration, eventReference);

   if (GameplayTag.IsValid())
   {
      if (UOSEAbilitySystemComponent* asc = _GetAbilitySystemComponentFromMesh(meshComp))
      {
         asc->AddLooseGameplayTag(GameplayTag, TagCount);
      }
   }
}

void UAnimNotifyState_LooseGameplayTag::NotifyTick(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float frameDeltaTime, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyTick(meshComp, animation, frameDeltaTime, eventReference);
}

void UAnimNotifyState_LooseGameplayTag::NotifyEnd(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyEnd(meshComp, animation, eventReference);

   if (GameplayTag.IsValid())
   {
      if (UOSEAbilitySystemComponent* asc = _GetAbilitySystemComponentFromMesh(meshComp))
      {
         asc->RemoveLooseGameplayTag(GameplayTag, TagCount);
      }
   }
}

UOSEAbilitySystemComponent* UAnimNotifyState_LooseGameplayTag::_GetAbilitySystemComponentFromMesh(USkeletalMeshComponent* meshComp)
{
   UOSEAbilitySystemComponent* asc = nullptr;
   if (meshComp)
   {
      asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(meshComp->GetOwner());
   }
   return asc;
}

