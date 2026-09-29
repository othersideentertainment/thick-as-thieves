// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Abilities/TATGameplayAbility_DodgeTrigger.h"

// tat
#include "Abilities/TATGameplayAbilityTargetData_Dodge.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Character/OSECharacterMovement.h"

// ue
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameplayAbility_DodgeTrigger)
DEFINE_LOG_CATEGORY_STATIC(LogTATGameplayAbility_DodgeTrigger, Log, All);

UTATGameplayAbility_DodgeTrigger::UTATGameplayAbility_DodgeTrigger()
{
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalOnly;
}

bool UTATGameplayAbility_DodgeTrigger::CanActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayTagContainer* sourceTags /*= nullptr*/, const FGameplayTagContainer* targetTags /*= nullptr*/, OUT FGameplayTagContainer* optionalRelevantTags /*= nullptr*/) const
{
   if (!Super::CanActivateAbility(handle, actorInfo, sourceTags, targetTags, optionalRelevantTags))
   {
      return false;
   }

   UOSECharacterMovement* movement = Cast< UOSECharacterMovement>(actorInfo->MovementComponent);
   if(movement == nullptr || !movement->CanDodgeInCurrentState())
   {
      return false;
   }

   APawn* avatar = Cast<APawn>(actorInfo->AvatarActor.Get());
   if (avatar == nullptr) return false;

   if (AllowForwardsDodge)
   {
      return true;
   }

   FVector viewDirectionVector = avatar->GetViewRotation().Vector();
   if (viewDirectionVector.IsNearlyZero())
   {
      return false;
   }

   FVector acceleration = movement->GetCurrentAcceleration();
   acceleration /= movement->GetMaxAcceleration();

   // only allow dodging if input would not be forwards
   const float dot = acceleration.GetSafeNormal2D() | viewDirectionVector.GetSafeNormal2D();
   bool withinForwardAngle = dot >= FMath::Cos(FMath::DegreesToRadians(AccelForwardHalfAngle));

   return !withinForwardAngle;
}

void UTATGameplayAbility_DodgeTrigger::ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData)
{
   Super::ActivateAbility(handle, actorInfo, activationInfo, triggerEventData);

   UAbilitySystemComponent* const asc = actorInfo->AbilitySystemComponent.Get();
   if (asc && CommitAbility(handle, actorInfo, activationInfo))
   {
      FScopedPredictionWindow newScopedWindow(asc, true);
      FGameplayEventData payload;

      // Check for dodge target data passed to ability
      bool providedDodgeTargetData = false;
      if (triggerEventData)
      {
         const int32 index = 0;
         if (const FGameplayAbilityTargetData* targetData = const_cast<FGameplayAbilityTargetData*>(triggerEventData->TargetData.Get(index)))
         {
            if (targetData->GetScriptStruct() == FTATGameplayAbilityTargetData_Dodge::StaticStruct())
            {
               providedDodgeTargetData = true;
               payload.TargetData.Append(triggerEventData->TargetData);
            }
            else
            {
               UE_LOG(LogTATGameplayAbility_DodgeTrigger, Error, TEXT("ActivateAbility() called with target data of unexpected type %s! Falling back to dodge using player's input direction...")
                  , *targetData->ToString());
            }
         }
      }

      // If none passed, create our own from character's movement direction
      if (!providedDodgeTargetData)
      {
         if (actorInfo->MovementComponent.IsValid())
         {
            const UOSECharacterMovement* movement = CastChecked<UOSECharacterMovement>(actorInfo->MovementComponent);
            FTATGameplayAbilityTargetData_Dodge* dodgeTargetData = FTATGameplayAbilityTargetData_Dodge::MakeFromCharacterMovement(movement);
            check(dodgeTargetData);
            payload.TargetData.Add(dodgeTargetData);
         }
         else
         {
            UE_LOG(LogTATGameplayAbility_DodgeTrigger, Error, TEXT("Invalid MovementComponent for actor %s! Could not generate dodge target data"), *GetNameSafe(actorInfo->AvatarActor.Get()));
         }
      }

      if (payload.TargetData.Num() > 0)
      {
         asc->HandleGameplayEvent(DodgeEventTag, &payload);
      }
   }

   const bool replicateEndAbility = false;
   const bool wasCancelled = false;
   EndAbility(handle, actorInfo, activationInfo, replicateEndAbility, wasCancelled);
}
