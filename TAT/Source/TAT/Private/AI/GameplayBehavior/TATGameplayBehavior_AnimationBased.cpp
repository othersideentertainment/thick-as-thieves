// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "TATGameplayBehavior_AnimationBased.h"

#include "GameplayBehaviorConfig_Animation.h"
#include "TATGameplayBehaviorConfig_Animation.h"
#include "Engine/AssetManager.h"

DEFINE_LOG_CATEGORY(LogTATGameplayBehavior_AnimationBased);

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayBehavior_AnimationBased)

void UTATGameplayBehavior_AnimationBased::AboutToTrigger_Implementation(AActor* avatar, AActor* smartObjectOwner)
{
   // Empty for now, however we may want to do some setup / teardown here in the future.
}

bool UTATGameplayBehavior_AnimationBased::Trigger(AActor& avatar, const UGameplayBehaviorConfig* config,
   AActor* smartObjectOwner)
{
   _bHasEndedBehavior = false;
   const UTATGameplayBehaviorConfig_Animation* animConfig = Cast<const UTATGameplayBehaviorConfig_Animation>(config);
   if (animConfig == nullptr)
   {
      UE_LOG(LogTATGameplayBehavior_AnimationBased, Error, TEXT("Config not correctly setup on gameplay behavior %s"), *this->GetName());
      return false;
   }
   if(animConfig->GetMontage() == nullptr)
   {
      // Spawn instance of loot actor at player's location
      TWeakObjectPtr<UTATGameplayBehavior_AnimationBased> weakThis(this);
      TWeakObjectPtr<AActor> weakSmartObjectOwner(smartObjectOwner);
      TWeakObjectPtr<AActor> weakAvatar(&avatar);
      TWeakObjectPtr<const UTATGameplayBehaviorConfig_Animation> weakConfig(animConfig);
      UAssetManager::GetStreamableManager().RequestAsyncLoad(animConfig->GetAnimMontage().ToSoftObjectPath(), [weakThis, weakSmartObjectOwner, weakAvatar, weakConfig]
      {
         if (weakThis.IsValid() && weakSmartObjectOwner.IsValid() && weakAvatar.IsValid() && weakConfig.IsValid())
         {
            if(const UTATGameplayBehaviorConfig_Animation* animConfig = weakConfig.Get())
            {
               weakThis->AnimationLoadedForAvatar(*weakAvatar.Get(), *animConfig->GetMontage(), animConfig, weakSmartObjectOwner.Get());
            }
         }
      });
      return true;
   }
   AnimationLoadedForAvatar(avatar, *animConfig->GetMontage(), config, smartObjectOwner);
   return true;
}

void UTATGameplayBehavior_AnimationBased::EndBehavior(AActor& Avatar, const bool bInterrupted)
{
   Super::EndBehavior(Avatar, bInterrupted);
   _bHasEndedBehavior = true;
}

void UTATGameplayBehavior_AnimationBased::AnimationLoadedForAvatar(AActor& actor, UAnimMontage& animMontage, const UGameplayBehaviorConfig* config, AActor* smartObjectOwner)
{
   if(_bHasEndedBehavior)
   {
      return;
   }
   AboutToTrigger(&actor, smartObjectOwner);
   PlayMontage(actor, animMontage);
   K2_OnTriggered(&actor, config, smartObjectOwner);
}
