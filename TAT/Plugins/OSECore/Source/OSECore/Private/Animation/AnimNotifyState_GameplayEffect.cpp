// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/AnimNotifyState_GameplayEffect.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Character/OSECharacterBase.h"

// ue4
#include "Components/SkeletalMeshComponent.h"
#include "Animation/AnimSequence.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_GameplayEffect)

UAnimNotifyState_GameplayEffect::UAnimNotifyState_GameplayEffect(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

FString UAnimNotifyState_GameplayEffect::GetNotifyName_Implementation() const
{
   FString notifyName = Super::GetNotifyName_Implementation();
   if (GameplayEffectClass)
   {
      notifyName = FString::Printf(TEXT("%s: %s"), *notifyName, *GameplayEffectClass.Get()->GetName());
   }
   else
   {
      notifyName = FString::Printf(TEXT("%s: NONE"), *notifyName);
   }
   return notifyName;
}

void UAnimNotifyState_GameplayEffect::NotifyBegin(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float totalDuration, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyBegin(meshComp, animation, totalDuration, eventReference);

   if (GameplayEffectClass)
   {
      if (UOSEAbilitySystemComponent* asc = _GetAbilitySystemComponentFromMesh(meshComp))
      {
         FGameplayEffectContextHandle effectContext = asc->MakeEffectContext();
         if (effectContext.IsValid())
         {
            UGameplayEffect* gameplayEffect = GameplayEffectClass->GetDefaultObject<UGameplayEffect>();
            asc->ApplyGameplayEffectToSelf(gameplayEffect, UGameplayEffect::INVALID_LEVEL, effectContext);
         }
      }
   }
}

void UAnimNotifyState_GameplayEffect::NotifyTick(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float frameDeltaTime, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyTick(meshComp, animation, frameDeltaTime, eventReference);
}

void UAnimNotifyState_GameplayEffect::NotifyEnd(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyEnd(meshComp, animation, eventReference);

   if (GameplayEffectClass)
   {
      if (UOSEAbilitySystemComponent* asc = _GetAbilitySystemComponentFromMesh(meshComp))
      {
         asc->RemoveActiveGameplayEffectBySourceEffect(GameplayEffectClass, nullptr);
      }
   }
}

UOSEAbilitySystemComponent* UAnimNotifyState_GameplayEffect::_GetAbilitySystemComponentFromMesh(USkeletalMeshComponent* meshComp)
{
   UOSEAbilitySystemComponent* asc = nullptr;
   if (meshComp)
   {
      asc = UOSEAbilitySystemComponent::GetOSEAbilitySystemComponent(meshComp->GetOwner());
   }
   return asc;
}

