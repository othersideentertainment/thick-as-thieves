// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/TATGameplayAbility_PossessPawn.h"

// ue5
#include "GameFramework/Controller.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_PossessPawn)

DEFINE_LOG_CATEGORY_STATIC(LogTATAbilityPossessPawn, Log, All);

UTATGameplayAbility_PossessPawn::UTATGameplayAbility_PossessPawn()
{
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UTATGameplayAbility_PossessPawn::AuthoritySwitchPossessionToNewPawn(APawn* pawn)
{
   AActor* avatarActor = GetAvatarActorFromActorInfo();

   if (!HasAuthority(&CurrentActivationInfo))
   {
      UE_LOG(LogTATAbilityPossessPawn, Error, TEXT("Calling AuthoritySwitchPossessionToNewPawn() on '%s' without authority"),
         *avatarActor->GetName());
   }
   else if (!avatarActor)
   {
      UE_LOG(LogTATAbilityPossessPawn, Error, TEXT("Calling AuthoritySwitchPossessionToNewPawn() on an actor that is null or PendingKill"));
   }
   else if (_isPossessingNewPawn)
   {
      UE_LOG(LogTATAbilityPossessPawn, Error, TEXT("Calling AuthoritySwitchPossessionToNewPawn() on '%s' while it's already controlling a new pawn already"),
         *avatarActor->GetName());
   }
   else if (!IsValid(pawn))
   {
      UE_LOG(LogTATAbilityPossessPawn, Error, TEXT("Calling AuthoritySwitchPossessionToNewPawn() on '%s' but passing a null pawn"),
         *avatarActor->GetName());
   }
   else
   {
      if (APawn* avatarPawn = Cast<APawn>(avatarActor))
      {
         if (AController* avatarController = avatarPawn->GetController())
         {
            _oldPawn = avatarPawn;
            _oldController = avatarController;
            _newPawn = pawn;

            avatarController->Possess(pawn);
            _isPossessingNewPawn = true;
         }
         else
         {
            UE_LOG(LogTATAbilityPossessPawn, Error, TEXT("Calling AuthoritySwitchPossessionToNewPawn() on '%s' which has no controller"),
               *avatarActor->GetName());
         }
      }
      else
      {
         UE_LOG(LogTATAbilityPossessPawn, Error, TEXT("Calling AuthoritySwitchPossessionToNewPawn() on '%s' which is not a pawn"),
            *avatarActor->GetName());
      }
   }
}

void UTATGameplayAbility_PossessPawn::AuthoritySwitchPossessionBackToPreviousPawn()
{
   AActor* avatarActor = GetAvatarActorFromActorInfo();

   if (!HasAuthority(&CurrentActivationInfo))
   {
      UE_LOG(LogTATAbilityPossessPawn, Error, TEXT("Calling AuthoritySwitchPossessionBackToPreviousPawn() on '%s' without authority"),
         *GetNameSafe(avatarActor));
   }
   else if (!avatarActor)
   {
      UE_LOG(LogTATAbilityPossessPawn, Error, TEXT("Calling AuthoritySwitchPossessionBackToPreviousPawn() on an actor that is null or PendingKill"));
   }
   if (!_isPossessingNewPawn)
   {
      UE_LOG(LogTATAbilityPossessPawn, Error, TEXT("Calling AuthoritySwitchPossessionBackToPreviousPawn() on '%s' while it's not currently controlling a new pawn"),
         *GetNameSafe(avatarActor));
   }
   else
   {
      if (IsValid(_oldController))
      {
         _oldController->UnPossess();

         if (IsValid(_oldPawn))
         {
            if (AActor* owningActor = GetOwningActorFromActorInfo())
            {
               // If the owning actor is being destroyed, calling Possess() will trigger assertions in FGameplayAbilityActorInfo::InitFromActor()
               // since we try to bring the ASC back up on an actor that's going away
               if (!owningActor->IsActorBeingDestroyed())
               {
                  _oldController->Possess(_oldPawn);
               }
            }
         }
      }

      _isPossessingNewPawn = false;
   }
}


