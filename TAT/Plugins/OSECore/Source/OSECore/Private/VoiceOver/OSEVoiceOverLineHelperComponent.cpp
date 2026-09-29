// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "VoiceOver/OSEVoiceOverLineHelperComponent.h"
#include <AbilitySystemInterface.h>

// ose
#include "Online/OSEGameState.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEVoiceOverLineHelperComponent)

#if WITH_EDITOR
EDataValidationResult UOSEGameplayTagVOLineDataAsset::IsDataValid(FDataValidationContext& context)
{
   for(const FOSEGameplayTagToVOLine& tagstoVoiceOvers : VoiceOvers)
   {
      if (!tagstoVoiceOvers.GameplayTag.IsValid())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has invalid gameplay tag \"%s\""), *GetName(), *tagstoVoiceOvers.GameplayTag.ToString())));
      }
      if (!tagstoVoiceOvers.RequestParams.Line)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has null line in entry \"%s\"!"), *GetName(), *tagstoVoiceOvers.GameplayTag.ToString())));
      }      
   }

   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif

UOSEVoiceOverLineHelperComponent::UOSEVoiceOverLineHelperComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
}

void UOSEVoiceOverLineHelperComponent::BeginPlay()
{
   Super::BeginPlay();
   
   if (GetOwner()->HasAuthority())
   {
      _AuthorityBindToAbilitySystemComponent();
   }
}

void UOSEVoiceOverLineHelperComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);

   if (GetOwner()->HasAuthority())
   {
      _AuthorityUnbindFromAbilitySystemComponent();
   }
}

void UOSEVoiceOverLineHelperComponent::_AuthorityBindToAbilitySystemComponent()
{
   check(GetOwner()->HasAuthority());

   if (UAbilitySystemComponent* asc = _GetAbilitySystemComponent())
   {
      _ForEachTagToVOLine([this, asc](const FOSEGameplayTagToVOLine& tagToVOLine)
         {
            asc->RegisterGameplayTagEvent(tagToVOLine.GameplayTag, EGameplayTagEventType::AnyCountChange).AddUObject(this, &UOSEVoiceOverLineHelperComponent::_AuthorityOnGameplayTagChanged);
         }
      );
   }
}

void UOSEVoiceOverLineHelperComponent::_AuthorityUnbindFromAbilitySystemComponent()
{
   check(GetOwner()->HasAuthority());

   if (UAbilitySystemComponent* asc = _GetAbilitySystemComponent())
   {
      _ForEachTagToVOLine([this, asc](const FOSEGameplayTagToVOLine& tagToVOLine)
         {
            asc->RegisterGameplayTagEvent(tagToVOLine.GameplayTag, EGameplayTagEventType::AnyCountChange).RemoveAll(this);
         }
      );
   }
}

UAbilitySystemComponent* UOSEVoiceOverLineHelperComponent::_GetAbilitySystemComponent() const
{
   if (IAbilitySystemInterface* const asi = Cast<IAbilitySystemInterface>(GetOwner()))
   {
      return asi->GetAbilitySystemComponent();
   }
   return nullptr;
}

void UOSEVoiceOverLineHelperComponent::_ForEachTagToVOLine(const TFunction<void(const FOSEGameplayTagToVOLine&)>& cb) const
{
   for(UOSEGameplayTagVOLineDataAsset* gameplayTagVOLineAsset : GameplayTagVOLineAssets)
   {
      if (!gameplayTagVOLineAsset)
         continue;

      for(const FOSEGameplayTagToVOLine& vo : gameplayTagVOLineAsset->VoiceOvers)
      {
         cb(vo);
      }
   }
}

void UOSEVoiceOverLineHelperComponent::_AuthorityOnGameplayTagChanged(const FGameplayTag tag, int32 newCount)
{
   check(GetOwner()->HasAuthority());

   _ForEachTagToVOLine([this, tag, newCount](const FOSEGameplayTagToVOLine& tagToVOLine)
      {
         if (tagToVOLine.GameplayTag == tag)
         {
            const int currentCount = _trackedTagCounts.GetTagCount(tag);
            const bool isAdd = currentCount <= 0 && newCount > 0;
            const bool isRemove = currentCount > 0 && newCount == 0;
            const bool isIncrement = !isAdd && newCount > currentCount;
            const bool isDecrement = !isRemove && newCount < currentCount;
            
            const bool broadcastOnAdd = tagToVOLine.TriggersOnFlags & (uint8)EOSEGameplayTagToVOLineTriggerFlags::CountAdd;
            const bool broadcastOnIncrement = tagToVOLine.TriggersOnFlags & (uint8)EOSEGameplayTagToVOLineTriggerFlags::CountIncrement;
            const bool broadcastOnDecrement = tagToVOLine.TriggersOnFlags & (uint8)EOSEGameplayTagToVOLineTriggerFlags::CountDecrement;
            const bool broadcastOnRemove = tagToVOLine.TriggersOnFlags & (uint8)EOSEGameplayTagToVOLineTriggerFlags::CountRemove;

            if ((isAdd && broadcastOnAdd) ||
                (isIncrement && broadcastOnIncrement) ||
                (isDecrement && broadcastOnDecrement) ||
                (isRemove && broadcastOnRemove))
            {
               AuthorityOnTriggerVoiceLine.Broadcast(tagToVOLine.RequestParams);
            }
         }
      }
   );

   // keep track for the next update
   _trackedTagCounts.SetTagCount(tag, newCount);
}

