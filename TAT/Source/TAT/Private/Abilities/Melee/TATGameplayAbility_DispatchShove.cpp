// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/Melee/TATGameplayAbility_DispatchShove.h"

// tat
#include "Tools/TATToolFunctionLibrary.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Combat/CombatComponent.h"

// ue
#include "AbilitySystemComponent.h"
#include "Animation/AnimMontage.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_DispatchShove)
DEFINE_LOG_CATEGORY_STATIC(LogTATGameplayAbility_DispatchShove, Log, All);

UTATGameplayAbility_DispatchShove::UTATGameplayAbility_DispatchShove()
{
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
   ActivationRequiresController = true;
   ActivationRequiresLocalControl = true;
}

#if WITH_EDITOR
EDataValidationResult UTATGameplayAbility_DispatchShove::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (!_shoveAbilityGameplayEvent.IsValid())
   {
      context.AddError(FText::FromString(TEXT("UTATGameplayAbility_DispatchShove has invalid _shoveAbilityGameplayEvent!")));
   }
   if (_shoveMontages.IsEmpty())
   {
      context.AddError(FText::FromString(TEXT("UTATGameplayAbility_DispatchShove has empty _shoveMontages!")));
   }
   else
   {
      for (auto montageIter = _shoveMontages.CreateConstIterator(); montageIter; ++montageIter)
      {
         const UAnimMontage* animMontage = *montageIter;
         if (animMontage == nullptr)
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("UTATGameplayAbility_DispatchShove | invalid entry in _shoveMontages at index %d!"), montageIter.GetIndex())));
         }
      }
   }

   return context.GetIssues().Num() > 0 ? EDataValidationResult::Invalid : result;
}
#endif // WITH_EDITOR

bool UTATGameplayAbility_DispatchShove::CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags, const FGameplayTagContainer* targetTags, OUT FGameplayTagContainer* optionalRelevantTags) const
{
   const AOSECharacterBase* character = Cast<AOSECharacterBase>(actorInfo->AvatarActor.Get());
   if (character == nullptr)
   {
      UE_LOG(LogTATGameplayAbility_DispatchShove, Error, TEXT("CanActivateAbility() failed for %s due to missing character!"), *actorInfo->OwnerActor->GetName());
      return false;
   }

   const UCombatComponent* combatComponent = character->GetCombatComponent();
   check(combatComponent);
   return !combatComponent->HasUninterruptibleCombatAnimation();
}

void UTATGameplayAbility_DispatchShove::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* ownerInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, ownerInfo, activationInfo, triggerEventData);

   if (!_shoveAbilityGameplayEvent.IsValid())
   {
      UE_LOG(LogTATGameplayAbility_DispatchShove, Error, TEXT("[%s] No assigned _shoveAbility tag! Could not trigger shove ability"), *GetName());
   }
   else if (const UAnimMontage* animMontage = _SelectShoveMontage())
   {
      if (CommitAbility(handle, ownerInfo, activationInfo))
      {
         // Pass along shove montage through event data
         FGameplayEventData payloadData;
         payloadData.OptionalObject = animMontage;

         UAbilitySystemComponent* asc = GetAbilitySystemComponentFromActorInfo_Ensured();
         FScopedPredictionWindow newScopedWindow(asc, true);
         const int32 activationCount = asc->HandleGameplayEvent(_shoveAbilityGameplayEvent, &payloadData);
         if (activationCount > 0)
         {
            UE_LOG(LogTATGameplayAbility_DispatchShove, Verbose, TEXT("Activated %d abilities"), activationCount);
         }
         else
         {
            UE_LOG(LogTATGameplayAbility_DispatchShove, Display, TEXT("Didn't activate any abilities for shove (probably stamina)"), activationCount);
         }
      }
   }

   constexpr bool replicateEndAbility = false;
   constexpr bool wasCancelled = false;
   EndAbility(handle, ownerInfo, activationInfo, replicateEndAbility, wasCancelled);
}

const UAnimMontage* UTATGameplayAbility_DispatchShove::_SelectShoveMontage() const
{
   if (_shoveMontages.Num() > 0)
   {
      const uint32 animMontageIndex = FMath::RandHelper(_shoveMontages.Num());
      const UAnimMontage* shoveMontage = _shoveMontages[animMontageIndex];
      UE_CLOG(shoveMontage == nullptr, LogTATGameplayAbility_DispatchShove, Error, TEXT("[%s] selected invalid AnimMontage at index %d!"), *GetName(), animMontageIndex);
      return shoveMontage;
   }

   UE_LOG(LogTATGameplayAbility_DispatchShove, Error, TEXT("[%s] has no entries in _shoveMontages!"), *GetName());
   return nullptr;
}
