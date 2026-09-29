// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATGameplayAbility_Thiefsign.h"

// tat
#include "Player/TATCharacter.h"
#include "Thiefsign/TATThiefsignFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_Thiefsign)
DEFINE_LOG_CATEGORY_STATIC(LogTATGameplayAbility_Thiefsign, Log, All);

UTATGameplayAbility_Thiefsign::UTATGameplayAbility_Thiefsign()
{
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UTATGameplayAbility_Thiefsign::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   if (const AActor* avatarActor = GetAvatarActorFromActorInfo())
   {
      _characterConfig = UTATThiefsignSettings::Get().FindCharacterConfig(avatarActor->GetClass());
   }
   else
   {
      UE_LOG(LogTATGameplayAbility_Thiefsign, Error, TEXT("ActivateAbility() found a null avatar actor!"));
      const bool replicateEndAbility = false;
      const bool wasCancelled = false;
      EndAbility(handle, ownerInfo, activationInfo, replicateEndAbility, wasCancelled);
   }

   Super::ActivateAbility(handle, ownerInfo, activationInfo, triggerEventData);
}

UAnimMontage* UTATGameplayAbility_Thiefsign::GetAnimation(const FGameplayEventData& triggerEventData, ETATThiefsignType type) const
{
   const UTATThiefsignSettings& thiefsignSettings = UTATThiefsignSettings::Get();

   FGameplayTag typeTag = FGameplayTag::EmptyTag;
   if (type == ETATThiefsignType::Decal)
   {
      typeTag = thiefsignSettings.DecalAnimationTag;
   }
   else if (type == ETATThiefsignType::Hand)
   {
      typeTag = thiefsignSettings.HandAnimationTag;
   }
   else if(type == ETATThiefsignType::Emote)
   {
      UTATThiefsignFunctionLibrary::RetrieveThiefsignAnimationIdentifierFromEventData(triggerEventData, typeTag);
   }
   else
   {
      checkNoEntry();
   }

   UAnimMontage* animation = nullptr;
   if (ATATCharacter* tatCharacter = Cast<ATATCharacter>(GetAvatarActorFromActorInfo()))
   {
      animation = tatCharacter->GetCharacterMontage(typeTag);
   }

   UE_CLOG(animation == nullptr, LogTATGameplayAbility_Thiefsign, Error, TEXT("GetAnimation() could not find %s character animation!"), *typeTag.ToString());
   return animation;
}

void UTATGameplayAbility_Thiefsign::DetermineCueParams(ETATThiefsignType type, const FGameplayTag& thiefsignIdentifier, FGameplayTag& cueTag, FGameplayCueParameters& cueParams, bool& success) const
{
   success = false;

   const UTATThiefsignSettings& thiefsignSettings = UTATThiefsignSettings::Get();

   if (AActor* instigator = GetAvatarActorFromActorInfo())
   {
      if (type == ETATThiefsignType::Decal)
      {
         cueTag = thiefsignSettings.DecalGameplayCueTag;
         cueParams = UTATThiefsignFunctionLibrary::BuildThiefsignCueParameters(thiefsignIdentifier, instigator);
         success = true;
      }
      else if (type == ETATThiefsignType::Hand)
      {
         cueTag = thiefsignSettings.HandGameplayCueTag;
         cueParams = UTATThiefsignFunctionLibrary::BuildThiefsignCueParameters(thiefsignIdentifier, instigator);
         success = true;
      }
      else
      {
         checkNoEntry();
      }
   }
   else
   {
      UE_LOG(LogTATGameplayAbility_Thiefsign, Error, TEXT("DetermineCueParams() found a null avatar actor!"));
   }
}
