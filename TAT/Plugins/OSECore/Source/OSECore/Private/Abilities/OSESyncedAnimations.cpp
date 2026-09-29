// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Abilities/OSESyncedAnimations.h"

// ose
#include "OSECommon.h"
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "Character/AvatarScaleInterface.h"
#include "Character/OSECharacterBase.h"
#include "Combat/CombatFunctionLibrary.h"

// ue4
#include "Misc/DataValidation.h"
#include "AbilitySystemGlobals.h"
#include "DrawDebugHelpers.h"
#include "Animation/AnimMontage.h"
#include "Kismet/KismetMathLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESyncedAnimations)

DEFINE_LOG_CATEGORY(LogSyncedAnimations);

const uint32 ESyncedAnimationConstraintAllFlags = 0xff;

namespace SyncedAnimationHelpers
{
   static const FName kRootBoneName = TEXT("root");

   const USkeleton* GetSkeletonForActor(AActor* actor)
   {
      if (USkeletalMeshComponent* skeletalMeshComponent = UOSECommon::GetComponent<USkeletalMeshComponent>(actor))
      {
         if (USkinnedAsset* skinnedAsset = skeletalMeshComponent->GetSkinnedAsset())
         {
            return skinnedAsset->GetSkeleton();
         }
      }
      return nullptr;
   }

   // TODO: make general utlity method?
   float GetAvatarScale(const AActor* actor)
   {
      if (const IAvatarScaleInterface* scaleInterface = Cast<IAvatarScaleInterface>(actor))
      {
         return scaleInterface->GetAvatarScale();
      }
      return 1;
   }

   FVector GetActorPosition(ESyncedAnimationAlignmentOrigin mode, const AActor* actor)
   {
      const ACharacter* character = Cast<ACharacter>(actor);
      if (character == nullptr)
      {
         return actor->GetActorLocation();
      }

      switch (mode)
      {
      case ESyncedAnimationAlignmentOrigin::Eyes:
         return character->GetPawnViewLocation();
      case ESyncedAnimationAlignmentOrigin::Feet:
         return character->GetNavAgentLocation();
      case ESyncedAnimationAlignmentOrigin::RootBone:
         {
            USkeletalMeshComponent* mesh = character->GetMesh();
            if (mesh && mesh->DoesSocketExist(kRootBoneName))
               return mesh->GetSocketTransform(kRootBoneName, ERelativeTransformSpace::RTS_World).GetLocation();
            else
               return character->GetActorLocation();
         }
         break;
      default:
         return character->GetActorLocation();
      }
   }

   float GetScaleForMode(ESyncedAnimationScaleMode mode, const AActor* source, const AActor* target)
   {
      switch (mode)
      {
      case ESyncedAnimationScaleMode::ScaleWithSource:
         return GetAvatarScale(source);
      case ESyncedAnimationScaleMode::ScaleWithTarget:
         return GetAvatarScale(target);
      case ESyncedAnimationScaleMode::None:
      default:
         return 1;
      }
   }

   UAnimMontage* GetCompatibleMontage(const AActor* actor, const FGameplayTag& animationTypeTag, EESyncedAnimationRole syncRole)
   {
      if (const AOSECharacterBase* character = Cast<AOSECharacterBase>(actor))
      {
         if (USyncedAnimationCharacterMontagesAsset* asset = character->GetSyncedAnimationCharacterMontagesAsset())
         {
            return asset->GetSyncedAnimationMontage(animationTypeTag, syncRole);
         }
      }
      return nullptr;
   }

   bool HasAvailableMontages(const AActor* actor, const FGameplayTag& animationTypeTag, EESyncedAnimationRole syncRole)
   {
      if (const AOSECharacterBase* character = Cast<AOSECharacterBase>(actor))
      {
         if (USyncedAnimationCharacterMontagesAsset* asset = character->GetSyncedAnimationCharacterMontagesAsset())
         {
            return asset->HasAvailableSyncedAnimationMontage(animationTypeTag, syncRole);
         }
      }
      return false;
   }

   // If Actor has an AbilitySystemComponent and a valid Avatar Actor, then return the Avatar. Otherwise return Actor.
   const AActor* GetAvatarOrActor(const AActor* actor)
   {
      UAbilitySystemComponent* actorAbilityComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(actor);
      const AActor* avatar = actorAbilityComponent ? actorAbilityComponent->GetAvatarActor() : nullptr;
      return avatar ? avatar : actor;
   }
}

//////////////////////////////////////////////////////////////////////////
// Constraints
//////////////////////////////////////////////////////////////////////////

bool USyncedAnimationConstraintBluePrintBase::CheckConstraintForUntargetedAnimation() const
{
   return CheckConstraintForUntargetedAnimationBlueprint();
}

bool USyncedAnimationConstraintBluePrintBase::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   return EvaluateConstraintBlueprint(parameters, syncedAnimation);
}

bool USyncedAnimationConstraintMoverRotation::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   if ((parameters.ConstraintFlags & UOSECommon::EnumToFlags(ESyncedAnimationConstraintFlags::Relax_Rotation)) == 0)
   {
      if (parameters.MoverDeltaRotator.Yaw < MinRotation.Yaw
         || parameters.MoverDeltaRotator.Pitch < MinRotation.Pitch
         || parameters.MoverDeltaRotator.Roll < MinRotation.Roll
         || parameters.MoverDeltaRotator.Yaw > MaxRotation.Yaw
         || parameters.MoverDeltaRotator.Pitch > MaxRotation.Pitch
         || parameters.MoverDeltaRotator.Roll > MaxRotation.Roll)
      {
         return false;
      }
   }

   return true;
}

bool USyncedAnimationConstraintMoverDistance::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   if ((parameters.ConstraintFlags & UOSECommon::EnumToFlags(ESyncedAnimationConstraintFlags::Relax_Distance) ) == 0)
   {
      return parameters.MoverDistance < MaxDistance;
   }
   return true;
}

bool USyncedAnimationConstraintMoverLateralDistance::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   if ((parameters.ConstraintFlags & UOSECommon::EnumToFlags(ESyncedAnimationConstraintFlags::Relax_Distance)) == 0)
   {
      return parameters.MoverLateralDistance < MaxDistance;
   }
   return true;
}

bool USyncedAnimationConstraintMoverHeight::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   return parameters.MoverHeight >= MinHeight && parameters.MoverHeight <= MaxHeight;
}

bool USyncedAnimationConstraintSourceTags::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   if (parameters.SourceAbilitySystemComponentTags.HasAny(BlockedTags))
   {
      return false;
   }

   if (!parameters.SourceAbilitySystemComponentTags.HasAll(RequiredTags))
   {
      return false;
   }
   return true;
}

bool USyncedAnimationConstraintTargetTags::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   if (parameters.TargetAbilitySystemComponentTags.HasAny(BlockedTags))
   {
      return false;
   }

   if (!parameters.TargetAbilitySystemComponentTags.HasAll(RequiredTags))
   {
      return false;
   }
   return true;
}

bool USyncedAnimationTargetSideConstraint::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   if ((parameters.ConstraintFlags & UOSECommon::EnumToFlags(ESyncedAnimationConstraintFlags::Relax_Side)) == 0)
   {
      FVector desiredDirection = parameters.DesiredRotation.Vector().GetSafeNormal();

      // use forward direction for our angle definition because it's easy to think in left/right degree space offset from 0
      FVector forwardDirection = parameters.MoverForward;
      float forwardDot = FVector::DotProduct(forwardDirection, desiredDirection);
      float forwardAngle = FMath::RadiansToDegrees(FMath::Acos(forwardDot));

      // use right direction for our left/right check so we can test against negative/positive
      FVector rightDirection = parameters.MoverRight;
      float rightDot = FVector::DotProduct(rightDirection, desiredDirection);

      float angle = FMath::Abs(AngleDegrees);
      switch(Side)
      {
      case ESyncedAnimationTargetSideType::Left:
         {
            return rightDot > 0.0f && forwardAngle >= angle;
         }
         break;
      case ESyncedAnimationTargetSideType::Right:
         {
            return rightDot < 0.0f && forwardAngle >= angle;
         }
         break;
      case ESyncedAnimationTargetSideType::Center:
         {
            return forwardAngle < angle;
         }
         break;
      }
      return false;
   }
   return true;
}

bool USyncedAnimationConstraintCombatHitPath::EvaluateConstraint(const FSyncedAnimationConstraintParameters& parameters, const FSyncedAnimationEntry& syncedAnimation) const
{
   const bool hasHitPath = UCombatFunctionLibrary::DoesAttackerHaveHitPathToDefender(parameters.Source, parameters.Target, Radius, DefenderTraceLogic);
   return hasHitPath;
}

//////////////////////////////////////////////////////////////////////////
// FSyncedAnimationEntry
//////////////////////////////////////////////////////////////////////////

FString FSyncedAnimationEntry::GetDebugName() const
{
   return AnimationTypeTag.ToString();
}

bool FSyncedAnimationEntry::MeetsConstraints(const FSyncedAnimationSearchParameters& parameters, bool requiresTarget) const
{
   const AActor* source = parameters.Source;
   const AActor* target = parameters.Target;

   // early-out case, the interaction system will make this query and we can skip a bunch of work...
   if (source == target)
      return false;

   UAbilitySystemComponent* sourceAbilityComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(source);
   UAbilitySystemComponent* targetAbilityComponent = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(target);

   FSyncedAnimationConstraintParameters constraintParameters;

   constraintParameters.Source = source;
   constraintParameters.Target = target;
   constraintParameters.ConstraintFlags = parameters.ConstraintFlags;

   sourceAbilityComponent->GetOwnedGameplayTags(constraintParameters.SourceAbilitySystemComponentTags);
   if (targetAbilityComponent)
      targetAbilityComponent->GetOwnedGameplayTags(constraintParameters.TargetAbilitySystemComponentTags);

   constraintParameters.SourcePosition = source->GetActorLocation();
   constraintParameters.SourceRotation = source->GetActorRotation();
   constraintParameters.SourceForward = source->GetActorForwardVector();
   constraintParameters.SourceRight = source->GetActorRightVector();

   if (target)
   {
      constraintParameters.TargetPosition = target->GetActorLocation();
      constraintParameters.TargetRotation = target->GetActorRotation();
      constraintParameters.TargetForward = target->GetActorForwardVector();
      constraintParameters.TargetRight = target->GetActorRightVector();
   }

   const FSyncedAnimationEntry& syncedAnimation = (*this);

   if (!syncedAnimation.bEnabled)
   {
      UE_LOG(LogSyncedAnimations, Verbose, TEXT("%s: skipping, not enabled"), *syncedAnimation.GetDebugName());
      return false;
   }

   if (requiresTarget && !target)
   {
      UE_LOG(LogSyncedAnimations, Verbose, TEXT("%s: skipping, requires a target but we don't have one"), *syncedAnimation.GetDebugName());
      return false;
   }

   const bool hasAvailableSourceMontage = SyncedAnimationHelpers::HasAvailableMontages(source, syncedAnimation.AnimationTypeTag, EESyncedAnimationRole::Source);
   const bool hasAvailableTargetMontage = SyncedAnimationHelpers::HasAvailableMontages(target, syncedAnimation.AnimationTypeTag, EESyncedAnimationRole::Target);

   if (RequireSourceAnimation && !hasAvailableSourceMontage)
   {
      UE_LOG(LogSyncedAnimations, Verbose, TEXT("%s: skipping, no compatible source montages"), *syncedAnimation.GetDebugName());
      return false;
   }

   // This check isn't valid -- we don't need a target montage to use an ability.  Previous iterations of this check were only ensuring
   // that if there were any montages available here, there was one for our current skeleton, but that's no longer an issue as we're pulling anims from characters.
   /*
   if (target && !hasAvailableTargetMontage)
   {
      UE_LOG(LogSyncedAnimations, Verbose, TEXT("%s: skipping, no compatible target montage"), *syncedAnimation.GetDebugName());
      return false;
   }
   */

   if (syncedAnimation.Configuration.SyncedAnimationTags.HasAny(parameters.BlockedTags))
   {
      UE_LOG(LogSyncedAnimations, Verbose, TEXT("%s: skipping, synced animation contains our blocked tags"), *syncedAnimation.GetDebugName());
      return false;
   }

   if (!syncedAnimation.Configuration.SyncedAnimationTags.HasAll(parameters.RequiredTags))
   {
      UE_LOG(LogSyncedAnimations, Verbose, TEXT("%s: skipping, synced animation doesn't contain all of our required tags"), *syncedAnimation.GetDebugName());
      return false;
   }

   constraintParameters.MoverPosition = constraintParameters.SourcePosition;
   constraintParameters.MoverRotation = constraintParameters.SourceRotation;
   constraintParameters.MoverForward = constraintParameters.SourceForward;
   constraintParameters.MoverRight = constraintParameters.SourceRight;

   if (target)
   {
      if (syncedAnimation.Alignment.Mode == ESyncedAnimationAlignmentMode::SourceForward || syncedAnimation.Alignment.Mode == ESyncedAnimationAlignmentMode::SourceToTarget)
      {
         constraintParameters.MoverPosition = constraintParameters.TargetPosition;
         constraintParameters.MoverRotation = constraintParameters.TargetRotation;
         constraintParameters.MoverForward = constraintParameters.TargetForward;
         constraintParameters.MoverRight = constraintParameters.TargetRight;
      }
   }

   if (USyncedAnimationFunctionLibrary::SyncedAnimation_CalculateAlignment(constraintParameters.DesiredPosition, constraintParameters.DesiredRotation, source, target, syncedAnimation.Alignment))
   {
      constraintParameters.MoverDeltaRotator = UKismetMathLibrary::NormalizedDeltaRotator(constraintParameters.DesiredRotation, constraintParameters.MoverRotation);
      constraintParameters.MoverDistance = FVector::Dist(constraintParameters.DesiredPosition, constraintParameters.MoverPosition);
      constraintParameters.MoverLateralDistance = FVector2D::Distance(FVector2D(constraintParameters.DesiredPosition.X, constraintParameters.DesiredPosition.Y), FVector2D(constraintParameters.MoverPosition.X, constraintParameters.MoverPosition.Y));
      constraintParameters.MoverHeight = (constraintParameters.MoverPosition.Z - constraintParameters.DesiredPosition.Z);

#if ENABLE_DRAW_DEBUG
      if (syncedAnimation.Debug)
      {
         DrawDebugCoordinateSystem(source->GetWorld(), constraintParameters.DesiredPosition, constraintParameters.DesiredRotation, 30, true, 30.0f, 0, 2.0f);
         DrawDebugCoordinateSystem(source->GetWorld(), constraintParameters.MoverPosition, constraintParameters.MoverRotation, 30, true, 30.0f, 0, 2.0f);
         DrawDebugDirectionalArrow(source->GetWorld(), constraintParameters.MoverPosition, constraintParameters.DesiredPosition, 30, FColor::Green, true, 30.0f, 0, 2.0f);
      }
#endif

      bool passedConstraints = true;

      for (USyncedAnimationConstraint* constraint : syncedAnimation.Constraints)
      {
         if (constraint)
         {
            // if we have no target, and dont need to check this constraint for an untargetted animation, we can skip evaluating this constraint
            if (!requiresTarget && !constraint->CheckConstraintForUntargetedAnimation())
               continue;

            if (!constraint->EvaluateConstraint(constraintParameters, syncedAnimation))
            {
               UE_LOG(LogSyncedAnimations, Verbose, TEXT("%s: constraint %s failed"), *syncedAnimation.GetDebugName(), *constraint->GetClass()->GetName());
               passedConstraints = false;
               break;
            }
            else
            {
               UE_LOG(LogSyncedAnimations, Verbose, TEXT("%s: constraint %s passed"), *syncedAnimation.GetDebugName(), *constraint->GetClass()->GetName());
            }
         }
         else
         {
            UE_LOG(LogSyncedAnimations, Error, TEXT("%s: NULL constraint found!"), *syncedAnimation.GetDebugName());
         }
      }

      if (!passedConstraints)
      {
         return false;
      }

      UE_LOG(LogSyncedAnimations, Verbose, TEXT("SyncedAnimation %s meets constraints"), *syncedAnimation.GetDebugName());
      return true;
   }
   else
   {
      UE_LOG(LogSyncedAnimations, Verbose, TEXT("%s: skipping, cannot calculate synced animation alignment"), *syncedAnimation.GetDebugName());
      return false;
   }
}

//////////////////////////////////////////////////////////////////////////
// USyncedAnimationFunctionLibrary
//////////////////////////////////////////////////////////////////////////

bool USyncedAnimationFunctionLibrary::SyncedAnimation_CalculateAlignment(FVector& position, FRotator& rotation, const AActor* source, const AActor* target, const FSyncedAnimationAlignment& syncedAnimationAlignment)
{
   if (!source)
   {
      return false;
   }

   if (!target)
   {
      FVector aimPos;
      FRotator aimRot;
      if (!UOSEAbilityFunctionLibrary::OffsetCameraAimToAvatarAim(source, FGameplayAbilityTargetingLocationInfo(), aimPos, aimRot))
      {
         // failed
         return false;
      }

      const AActor* avatarSource = SyncedAnimationHelpers::GetAvatarOrActor(source);
      position = avatarSource->GetActorLocation();
      rotation = aimRot;

      // success
      return true;
   }

   // If Source or Target have a valid Avatar, then use it instead.
   const AActor* actualSource = SyncedAnimationHelpers::GetAvatarOrActor(source);
   const AActor* actualTarget = SyncedAnimationHelpers::GetAvatarOrActor(target);

   const FVector rawSourcePosition = actualSource->GetActorLocation();
   const FVector rawTargetPosition = actualTarget->GetActorLocation();
   const FVector sourcePosition = SyncedAnimationHelpers::GetActorPosition(syncedAnimationAlignment.SourceOrigin, actualSource);
   const FVector targetPosition = SyncedAnimationHelpers::GetActorPosition(syncedAnimationAlignment.TargetOrigin, actualTarget);
   const FVector targetPositionForSource = targetPosition + (rawSourcePosition - sourcePosition);
   const FVector sourcePositionForTarget = sourcePosition + (rawTargetPosition - targetPosition);
   const FRotator sourceRotator = actualSource->GetActorRotation();
   const FRotator targetRotator = actualTarget->GetActorRotation();

   FVector targetToSource = sourcePosition - targetPosition;
   FVector targetToSourceDirection = targetToSource;
   targetToSource.Normalize();

   FVector relativePosition = syncedAnimationAlignment.RelativePosition * SyncedAnimationHelpers::GetScaleForMode(syncedAnimationAlignment.AvatarScaleMode, actualSource, actualTarget);

   switch(syncedAnimationAlignment.Mode) 
   {
   case ESyncedAnimationAlignmentMode::TargetToSource:
      {
         const FRotator alignRotator = UKismetMathLibrary::MakeRotFromX(targetToSource);
         position = targetPositionForSource + alignRotator.RotateVector(relativePosition);
         rotation = UKismetMathLibrary::ComposeRotators(alignRotator, syncedAnimationAlignment.RelativeRotation);
      }
      break;
   case ESyncedAnimationAlignmentMode::SourceToTarget:
      {
         const FRotator alignRotator = UKismetMathLibrary::MakeRotFromX(-targetToSource);
         position = sourcePositionForTarget + alignRotator.RotateVector(relativePosition);
         rotation = UKismetMathLibrary::ComposeRotators(alignRotator, syncedAnimationAlignment.RelativeRotation);
      }
      break;
   case ESyncedAnimationAlignmentMode::TargetForward:
      {
         position = targetPositionForSource + targetRotator.RotateVector(relativePosition);
         rotation = UKismetMathLibrary::ComposeRotators(targetRotator, syncedAnimationAlignment.RelativeRotation);
      }
      break;
   case ESyncedAnimationAlignmentMode::SourceForward:
   default:
      {
         position = sourcePositionForTarget + sourceRotator.RotateVector(relativePosition);
         rotation = UKismetMathLibrary::ComposeRotators(sourceRotator, syncedAnimationAlignment.RelativeRotation);
      }
      break;
   }
   return true;
}

UAnimMontage* USyncedAnimationFunctionLibrary::FindMontageForSyncedAnimation(const FSyncedAnimationEntry& entry, AActor* actor, EESyncedAnimationRole role)
{
   return SyncedAnimationHelpers::GetCompatibleMontage(actor, entry.AnimationTypeTag, role);
}

void USyncedAnimationFunctionLibrary::ExecuteActivateAbilityActions(const FSyncedAnimationEntry& entry, AActor* actor, EESyncedAnimationRole role, AActor* otherActor)
{
   const TArray<UEventTimelineAction*>& actions = role == EESyncedAnimationRole::Source ? entry.SourceActivateAbilityActions : entry.TargetActivateAbilityActions;
   for (UEventTimelineAction* eventTimelineAction : actions)
   {
      if (eventTimelineAction)
         eventTimelineAction->EvaluateAction(actor, otherActor);
   }
}

void USyncedAnimationFunctionLibrary::ExecuteAnimationCompleteActions(const FSyncedAnimationEntry& entry, AActor* actor, EESyncedAnimationRole role, AActor* otherActor)
{
   const TArray<UEventTimelineAction*>& actions = role == EESyncedAnimationRole::Source ? entry.SourceAnimationCompleteActions : entry.TargetAnimationCompleteActions;
   for (UEventTimelineAction* eventTimelineAction : actions)
   {
      if (eventTimelineAction)
         eventTimelineAction->EvaluateAction(actor, otherActor);
   }
}

void USyncedAnimationFunctionLibrary::ExecuteEndAbilityActions(const FSyncedAnimationEntry& entry, AActor* actor, EESyncedAnimationRole role, AActor* otherActor)
{
   const TArray<UEventTimelineAction*>& actions = role == EESyncedAnimationRole::Source ? entry.SourceEndAbilityActions : entry.TargetEndAbilityActions;
   for (UEventTimelineAction* eventTimelineAction : actions)
   {
      if (eventTimelineAction)
         eventTimelineAction->EvaluateAction(actor, otherActor);
   }
}

//////////////////////////////////////////////////////////////////////////
// FSyncedAnimationMontageSets
//////////////////////////////////////////////////////////////////////////

const TArray<UAnimMontage*>& FSyncedAnimationMontageSets::GetMontagesForRole(EESyncedAnimationRole role) const
{
   switch(role)
   {
   case EESyncedAnimationRole::Source:
      return SourceMontages;
   case EESyncedAnimationRole::Target:
      return TargetMontages;
   default:
      unimplemented();
      // assert above will fire before this returns anything
      return SourceMontages;
   }
}

//////////////////////////////////////////////////////////////////////////
// USyncedAnimationCharacterMontagesAsset
//////////////////////////////////////////////////////////////////////////

UAnimMontage* USyncedAnimationCharacterMontagesAsset::GetSyncedAnimationMontage(const FGameplayTag& animationTypeTag, EESyncedAnimationRole syncRole) const
{
   if (const FSyncedAnimationMontageSets* montageSets = _GetMontageSet(animationTypeTag))
   {
      const TArray<UAnimMontage*>& montages = montageSets->GetMontagesForRole(syncRole);

      // no anim defined on this character for this attack/role
      if (montages.Num() == 0)
         return nullptr;

      // TODO:
      // for now, rng?  future: LRU? some best-fit definitions?
      return montages[FMath::RandHelper(montages.Num())];
   }
   return nullptr;
}

bool USyncedAnimationCharacterMontagesAsset::HasAvailableSyncedAnimationMontage(const FGameplayTag& animationTypeTag, EESyncedAnimationRole syncRole) const
{
   if (const FSyncedAnimationMontageSets* montageSets = _GetMontageSet(animationTypeTag))
   {
      const TArray<UAnimMontage*>& montages = montageSets->GetMontagesForRole(syncRole);
      return montages.Num() > 0;
   }
   return false;
}

#if WITH_EDITOR
void USyncedAnimationCharacterMontagesAsset::ValidateForCharacter(const AOSECharacterBase* character, class FDataValidationContext& Context)
{
   check(character);

   // ignore characters that don't even have a proper skeleton...?
   USkinnedAsset* skinnedAsset = character->GetMesh() ? character->GetMesh()->GetSkinnedAsset() : nullptr;
   USkeleton* skeleton = skinnedAsset ? skinnedAsset->GetSkeleton() : nullptr;
   if (skeleton)
   {
      for (auto it = CharacterMontageSets.CreateConstIterator(); it; ++it)
      {
         const FSyncedAnimationMontageSets& entry = (*it);

         // TODO:  Two are fine, but if we need to dup this a 3rd time it's becoming a macro!

         // SourceMontages
         for (int idx = 0; idx < entry.SourceMontages.Num(); ++idx)
         {
            UAnimMontage* montage = entry.SourceMontages[idx];
            if (!montage)
            {
               Context.AddError(FText::FromString(FString::Printf(TEXT("[%s] SourceMontages has an empty/missing asset at index %d!"), *character->GetName(), idx)));
            }
            else if (montage->GetSkeleton() != skeleton)
            {
               Context.AddError(FText::FromString(FString::Printf(TEXT("[%s] SourceMontages points to a montage %s that is not valid for it's skeleton!"), *character->GetName(), *montage->GetName())));
            }
         }

         // TargetMontages
         for (int idx = 0; idx < entry.TargetMontages.Num(); ++idx)
         {
            UAnimMontage* montage = entry.TargetMontages[idx];
            if (!montage)
            {
               Context.AddError(FText::FromString(FString::Printf(TEXT("[%s] TargetMontages has an empty/missing asset at index %d!"), *character->GetName(), idx)));
            }
            else if (montage->GetSkeleton() != skeleton)
            {
               Context.AddError(FText::FromString(FString::Printf(TEXT("[%s] SourceMontages points to a montage %s that is not valid for it's skeleton!"), *character->GetName(), *montage->GetName())));
            }
         }
      }
   }
}
#endif

const FSyncedAnimationMontageSets* USyncedAnimationCharacterMontagesAsset::_GetMontageSet(const FGameplayTag& animationTypeTag) const
{
   for(const FSyncedAnimationMontageSets& montageSet : CharacterMontageSets)
   {
      if (animationTypeTag == montageSet.AnimationTypeTag)
         return &montageSet;
   }
   return nullptr;
}

