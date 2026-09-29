// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/Tasks/AbilityTask_PossessNewPawn.h"

// ue5
#include "GameplayTasksComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AbilityTask_PossessNewPawn)

DEFINE_LOG_CATEGORY_STATIC(LogTATAbilityTaskPossessNewPawn, Log, All);

UAbilityTask_PossessNewPawn* UAbilityTask_PossessNewPawn::AuthorityPossessNewPawn(
   UGameplayAbility* owningAbility,
   APawn* newPawnToPossess)
{
   UAbilityTask_PossessNewPawn* task = NewAbilityTask<UAbilityTask_PossessNewPawn>(owningAbility);
   task->_newPawn = newPawnToPossess;

   return task;
}

void UAbilityTask_PossessNewPawn::Activate()
{
   Super::Activate();

   AActor* avatarActor = Ability->GetAvatarActorFromActorInfo();

   if (!Ability->HasAuthority(&Ability->GetCurrentActivationInfoRef()))
   {
      UE_LOG(LogTATAbilityTaskPossessNewPawn, Error, TEXT("Using UAbilityTask_PossessNewPawn on '%s' without authority"),
         *avatarActor->GetName());
   }
   else if (!avatarActor)
   {
      UE_LOG(LogTATAbilityTaskPossessNewPawn, Error, TEXT("Using UAbilityTask_PossessNewPawn on an actor that is null or PendingKill"));
   }
   else if (Ability->GetGameplayTasksComponent(*this)->HasActiveTasks(UAbilityTask_PossessNewPawn::StaticClass()))
   {
      UE_LOG(LogTATAbilityTaskPossessNewPawn, Error, TEXT("Using UAbilityTask_PossessNewPawn on '%s' while it's already controlling a new pawn already"),
         *avatarActor->GetName());
   }
   else if (!IsValid(_newPawn))
   {
      UE_LOG(LogTATAbilityTaskPossessNewPawn, Error, TEXT("Using UAbilityTask_PossessNewPawn on '%s' but passing a null pawn"),
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
            avatarController->Possess(_newPawn);
         }
         else
         {
            UE_LOG(LogTATAbilityTaskPossessNewPawn, Error, TEXT("Using UAbilityTask_PossessNewPawn on '%s' which has no controller"),
               *avatarActor->GetName());
         }
      }
      else
      {
         UE_LOG(LogTATAbilityTaskPossessNewPawn, Error, TEXT("Using UAbilityTask_PossessNewPawn on '%s' which is not a pawn"),
            *avatarActor->GetName());
      }
   }
}

void UAbilityTask_PossessNewPawn::OnDestroy(bool abilityIsEnding)
{
   AActor* avatarActor = Ability->GetAvatarActorFromActorInfo();

   if (!Ability->HasAuthority(&Ability->GetCurrentActivationInfoRef()))
   {
      UE_LOG(LogTATAbilityTaskPossessNewPawn, Error, TEXT("Ending UAbilityTask_PossessNewPawn on '%s' without authority"),
         *avatarActor->GetName());
   }
   else if (!avatarActor)
   {
      UE_LOG(LogTATAbilityTaskPossessNewPawn, Error, TEXT("Ending UAbilityTask_PossessNewPawn on an actor that is null or PendingKill"));
   }
   else
   {
      if (IsValid(_oldController))
      {
         _oldController->UnPossess();

         if (IsValid(_oldPawn))
         {
            if (AActor* owningActor = Ability->GetOwningActorFromActorInfo())
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
   }

   Super::OnDestroy(abilityIsEnding);
}


