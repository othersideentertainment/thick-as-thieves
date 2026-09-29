// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Animation/AnimNotifyState_Combat.h"

// ose
#include "Animation/AnimMetadata_Combat.h"
#include "Character/OSECharacterBase.h"
#include "Combat/CombatComponent.h"

// ue4
#include "DrawDebugHelpers.h"
#include "Animation/AnimLinkableElement.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequence.h"
#include "Components/SkeletalMeshComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(AnimNotifyState_Combat)

namespace
{
   UCombatComponent* GetCombatComponent(USkeletalMeshComponent* meshComp)
   {
      // Potential future thing: If we add combat to non-OSECharacterBase characters we should add an interface for this getter.
      if (AOSECharacterBase* character = Cast<AOSECharacterBase>(meshComp->GetOwner()))
      {
         return character->GetCombatComponent();
      }
      return nullptr;
   }
}

namespace CombatCVars
{
   static int DebugDrawHitboxesEditor = 0;
   FAutoConsoleVariableRef CVarDebugCombatHitboxesEditor(
      TEXT("OSE.Combat.DebugDrawHitboxesEditor"),
      DebugDrawHitboxesEditor,
      TEXT("Draw combat debug information in the animation editor window"),
      ECVF_Default);
}

//---------------------------------------------------------------------------------------
// UAnimNotifyState_Combat
//---------------------------------------------------------------------------------------

UAnimNotifyState_Combat::UAnimNotifyState_Combat(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

FString UAnimNotifyState_Combat::GetNotifyName_Implementation() const
{
   return FString::Printf(TEXT("Combat: %s"), *_GetCombatNotifyName());
}

//---------------------------------------------------------------------------------------
// UAnimNotify_Combat
//---------------------------------------------------------------------------------------

UAnimNotify_Combat::UAnimNotify_Combat(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{

}

FString UAnimNotify_Combat::GetNotifyName_Implementation() const
{
   return FString::Printf(TEXT("Combat: %s"), *_GetCombatNotifyName());
}

void UAnimNotify_Combat::Notify(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference)
{
   // notifies that aren't driven by UAnimNotifyState_CombatMontage get their callbacks inline here
   if (!IsDrivenByCombatMontage())
   {
      if (meshComp && animation)
      {
         if (UCombatComponent* combatComp = GetCombatComponent(meshComp))
         {
            PerformCombatAction(*combatComp, *meshComp, *animation);
         }
      }
   }

   Super::Notify(meshComp, animation, eventReference);
}

//---------------------------------------------------------------------------------------
// UAnimNotify_CombatAnimationStart
//---------------------------------------------------------------------------------------

void UAnimNotify_CombatAnimationStart::PerformCombatAction(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation)
{
   if (UAnimMetadata_CombatHitboxes* meta = UAnimMetadata_CombatHitboxes::GetAnimMetadata(&animation))
   {
      combatComponent.OnCombatAnimationSequenceStart(animation, *meta);
   }
}

//---------------------------------------------------------------------------------------
// UAnimNotify_CombatAnimationEnd
//---------------------------------------------------------------------------------------

void UAnimNotify_CombatAnimationEnd::PerformCombatAction(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation)
{
   combatComponent.OnCombatAnimationSequenceEnd(animation);
}

//---------------------------------------------------------------------------------------
// UAnimNotify_CombatAutoAim
//---------------------------------------------------------------------------------------

void UAnimNotify_CombatAutoAim::PerformCombatAction(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation)
{
   if (UAnimMetadata_CombatHitboxes* meta = UAnimMetadata_CombatHitboxes::GetAnimMetadata(&animation))
   {
      combatComponent.OnCombatAnimationAutoAim(animation, *meta);
   }
}

//---------------------------------------------------------------------------------------
// UAnimNotify_CombatRangedProjectileSpawn
//---------------------------------------------------------------------------------------

void UAnimNotify_CombatRangedProjectileSpawn::PerformCombatAction(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation)
{
   combatComponent.OnCombatRangedProjectileSpawn(animation);
}

//---------------------------------------------------------------------------------------
// UAnimNotify_CombatRangedReload
//---------------------------------------------------------------------------------------

void UAnimNotify_CombatRangedReload::PerformCombatAction(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation)
{
   combatComponent.OnCombatRangedReload(animation);
}

//---------------------------------------------------------------------------------------
// UAnimNotifyState_CombatHitbox
//---------------------------------------------------------------------------------------

void UAnimNotifyState_CombatHitbox::NotifyBegin(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float totalDuration, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyBegin(meshComp, animation, totalDuration, eventReference);

#if ENABLE_DRAW_DEBUG
   if (UAnimMetadata_CombatHitboxes* combatHitboxesMeta = UAnimMetadata_CombatHitboxes::GetAnimMetadata(animation))
   {
      if (CombatCVars::DebugDrawHitboxesEditor)
      {
         const FCombatHitboxMetadata& hitbox = combatHitboxesMeta->GetMetadataForHitboxIndex(HitboxIndex);

         DrawDebugSphere(
            meshComp->GetWorld(),
            hitbox.Location,
            hitbox.Radius,
            25,
            hitbox.DebugTraceColor.ToFColor(true),
            false,
            1.0f,
            0,
            1.5);
      }
   }
#endif // ENABLE_DRAW_DEBUG
}

void UAnimNotifyState_CombatHitbox::PerformCombatActionBegin(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation)
{
   combatComponent.EnableHitbox(animation, HitboxIndex);
}

void UAnimNotifyState_CombatHitbox::PerformCombatActionEnd(UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, const UAnimSequenceBase& animation)
{
   combatComponent.DisableHitbox(animation, HitboxIndex);
}

//---------------------------------------------------------------------------------------
// UAnimNotifyState_CombatHitboxMontage
//---------------------------------------------------------------------------------------

void UAnimNotifyState_CombatMontage::NotifyBegin(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float totalDuration, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyBegin(meshComp, animation, totalDuration, eventReference);

   if (UCombatComponent* combatComponent = GetCombatComponent(meshComp))
   {
      combatComponent->SetLastMontagePosition(0.0f);
      combatComponent->OnCombatAnimationMontageStart(*animation, CombatAttackDirection);
   }
}

void UAnimNotifyState_CombatMontage::NotifyTick(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, float frameDeltaTime, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyTick(meshComp, animation, frameDeltaTime, eventReference);

   if (!meshComp)
      return;

   UCombatComponent* combatComponent = GetCombatComponent(meshComp);
   UAnimMontage* montage = Cast<UAnimMontage>(animation);
   UAnimInstance* animInstance = meshComp->GetAnimInstance();
   if (!combatComponent || !montage || !animInstance)
      return;

   if (animInstance->Montage_IsPlaying(montage))
   {
      const float montagePosition = animInstance->Montage_GetPosition(montage);
      const float lastMontagePosition = combatComponent->GetLastMontagePosition();
      _SendCombatNotifies(*montage, *combatComponent, *meshComp, lastMontagePosition, montagePosition);

      // cache for next run
      combatComponent->SetLastMontagePosition(montagePosition);
   }
}

void UAnimNotifyState_CombatMontage::NotifyEnd(USkeletalMeshComponent* meshComp, UAnimSequenceBase* animation, const FAnimNotifyEventReference& eventReference)
{
   Super::NotifyEnd(meshComp, animation, eventReference);

   if (!meshComp)
      return;

   UCombatComponent* combatComponent = GetCombatComponent(meshComp);
   UAnimMontage* montage = Cast<UAnimMontage>(animation);
   UAnimInstance* animInstance = meshComp->GetAnimInstance();
   if (!combatComponent || !montage || !animInstance)
      return;

   // send final notifies
   const float finalMontagePosition = montage->GetPlayLength();
   const float lastMontagePosition = combatComponent->GetLastMontagePosition();
   _SendCombatNotifies(*montage, *combatComponent, *meshComp, lastMontagePosition, finalMontagePosition);

   // done!
   combatComponent->OnCombatAnimationMontageEnd(*animation);
   combatComponent->SetLastMontagePosition(0.0f);
}

void UAnimNotifyState_CombatMontage::_SendCombatNotifies(UAnimMontage& montage, UCombatComponent& combatComponent, USkeletalMeshComponent& meshComp, float prevMontagePosition, float currentMontagePosition)
{
   int currentSectionIdx = montage.GetSectionIndexFromPosition(currentMontagePosition);
   if (currentSectionIdx != INDEX_NONE)
   {
      FCompositeSection& section = montage.CompositeSections[currentSectionIdx];
      if (const UAnimSequence* seq = Cast<UAnimSequence>(section.GetLinkedSequence()))
      {
         float sectionTimeStart, sectionTimeEnd;
         montage.GetSectionStartAndEndTime(currentSectionIdx, sectionTimeStart, sectionTimeEnd);
         const float currentSectionPosition = currentMontagePosition - sectionTimeStart;

         float lastSectionPosition = 0.0f;
         if (prevMontagePosition >= sectionTimeStart)
         {
            lastSectionPosition = prevMontagePosition - sectionTimeStart;
         }

         // get all the notifies that are currently active
         FAnimNotifyContext notifyContext;
         seq->GetAnimNotifiesFromDeltaPositions(lastSectionPosition, currentSectionPosition, notifyContext);
         
         for (const FAnimNotifyEventReference& notifyEventRef : notifyContext.ActiveNotifies)
         {
            if (const FAnimNotifyEvent* notifyEvent = notifyEventRef.GetNotify())
            {
               // combat notify
               if (notifyEvent->Notify && notifyEvent->Notify->IsA(UAnimNotify_Combat::StaticClass()))
               {
                  if (UAnimNotify_Combat* combatNotify = Cast<UAnimNotify_Combat>(notifyEvent->Notify))
                  {
                     combatNotify->PerformCombatAction(combatComponent, meshComp, *seq);
                  }
               }
               // combat state notify
               else if (notifyEvent->NotifyStateClass && notifyEvent->NotifyStateClass->IsA(UAnimNotifyState_Combat::StaticClass()))
               {
                  if (UAnimNotifyState_Combat* combatStateNotify = Cast<UAnimNotifyState_Combat>(notifyEvent->NotifyStateClass))
                  {
                     const float notifyStartTime = notifyEvent->GetTriggerTime();
                     const float notifyEndTime = notifyEvent->GetEndTriggerTime();

                     // begin / end only trigger once per notify state, tick is called each frame
                     const bool needsBegin = notifyStartTime >= lastSectionPosition;
                     const bool needsEnd = notifyEndTime < currentSectionPosition;

                     // begin
                     if (needsBegin)
                        combatStateNotify->PerformCombatActionBegin(combatComponent, meshComp, *seq);

                     // tick
                     combatStateNotify->PerformCombatActionTick(combatComponent, meshComp, *seq);

                     // end
                     if (needsEnd)
                        combatStateNotify->PerformCombatActionEnd(combatComponent, meshComp, *seq);
                  }
               }
            }
         }
      }
   }
}

