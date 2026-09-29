// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Graphics/Auras/TATAuraVisibilityTypes.h"

// tat
#include "Graphics/Auras/TATAuraVisibilityPerceiverComponent.h"
#include "Graphics/Auras/TATAuraVisibilityTargetComponent.h"

// ose
#include "Character/OSECharacterBase.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAuraVisibilityTypes)

DEFINE_LOG_CATEGORY_STATIC(LogTATAuraVisibilityData, Log, All);


FTATAuraVisibilityTargetCachedData::FTATAuraVisibilityTargetCachedData(TWeakObjectPtr<const UTATAuraVisibilityTargetComponent> auraTarget, const bool perceiverHasLineOfSight)
   : AuraTarget(auraTarget)
   , PerceiverHasLineOfSight(perceiverHasLineOfSight)
{
   check(auraTarget.IsValid());
}

FTATAuraVisibilityState::FTATAuraVisibilityState(TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> perceivingPlayer, FGameplayTag auraSenseTag, bool isPerceived)
   : PerceivingPlayer(perceivingPlayer)
   , AuraSenseTag(auraSenseTag)
   , _isPerceived(isPerceived)
{
   check(perceivingPlayer.IsValid());
}

bool FTATAuraVisibilityState::SetPerceived(UTATAuraVisibilityTargetComponent* perceivedTarget, bool isPerceived)
{
   if (_isPerceived != isPerceived) 
   {
      _isPerceived = isPerceived;

      // Start/stop persistence timer on perception change
      check(PerceivingPlayer.IsValid());
      const FTATAuraVisibilityPerceiverSense* auraSense = PerceivingPlayer->GetAuraSense(AuraSenseTag);
      check(auraSense);
      if (auraSense->AuraDurationAfterLastPerceived > 0)
      {
         if (_isPerceived)
         {
            if (_timerHandle.IsValid())
            {
               UE_LOG(LogTATAuraVisibilityData, Verbose, TEXT("%s | cleared persistence timer for perceiver %s (sense = %s)")
                  , *perceivedTarget->GetOwner()->GetName()
                  , *PerceivingPlayer->GetOwner()->GetName()
                  , *AuraSenseTag.ToString());

               ClearPersistenceTimer(perceivedTarget);
            }
         }
         else
         {
            const bool loopTimer = false;
            FTimerDelegate timerDelegate = FTimerDelegate::CreateUObject(perceivedTarget, &UTATAuraVisibilityTargetComponent::OnAuraPersistenceTimerElapsed, PerceivingPlayer, AuraSenseTag);

            check(IsValid(perceivedTarget));
            perceivedTarget->GetOwner()->GetWorld()->GetTimerManager().SetTimer(_timerHandle, timerDelegate, auraSense->AuraDurationAfterLastPerceived, loopTimer);

            UE_LOG(LogTATAuraVisibilityData, Verbose, TEXT("%s | started persistence timer for perceiver %s (sense = %s)")
               , *perceivedTarget->GetOwner()->GetName()
               , *PerceivingPlayer->GetOwner()->GetName()
               , *AuraSenseTag.ToString());
         }
      }

      return true;
   }

   return false;
}

bool FTATAuraVisibilityState::IsPerceived() const
{
   // If a persistence timer is assigned for this aura sense, return true if it's running
   return _isPerceived || _timerHandle.IsValid();
}

void FTATAuraVisibilityState::ClearPersistenceTimer(UTATAuraVisibilityTargetComponent* perceivedTarget)
{
   check(IsValid(perceivedTarget));
   perceivedTarget->GetWorld()->GetTimerManager().ClearTimer(_timerHandle);
}

bool FTATAuraVisibilityStateArray::UpdateAuraPerceiverSenseEntry(UTATAuraVisibilityTargetComponent* perceivedTarget, TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> perceivingPlayer, FGameplayTag auraSenseTag, bool isBeingPerceived)
{
   check(perceivingPlayer.IsValid());
   check(perceivingPlayer->ShouldEvaluateAnyAuraSenses());
   check(perceivingPlayer->ShouldEvaluateAuraSense(auraSenseTag));

   FTATAuraVisibilityState* auraVisibilityState = _GetAuraVisibilityState(perceivingPlayer, auraSenseTag);

   // Update existing state entry if present
   if (auraVisibilityState)
   {
      return auraVisibilityState->SetPerceived(perceivedTarget, isBeingPerceived);
   }
   // Otherwise create new entry
   else
   {
      FTATAuraVisibilityState newAuraVisibilityState(perceivingPlayer, auraSenseTag, isBeingPerceived);
      Items.Add(newAuraVisibilityState);
      return true;
   }
}

void FTATAuraVisibilityStateArray::ClearAuraSensePersistenceTimer(UTATAuraVisibilityTargetComponent* perceivedTarget, TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> perceivingPlayer, FGameplayTag auraSenseTag)
{
   check(perceivingPlayer.IsValid());

   FTATAuraVisibilityState* auraVisibilityState = _GetAuraVisibilityState(perceivingPlayer, auraSenseTag);
   check(auraVisibilityState);
   auraVisibilityState->ClearPersistenceTimer(perceivedTarget);
}

void FTATAuraVisibilityStateArray::RemoveAuraEntriesForPerceiver(UTATAuraVisibilityTargetComponent* perceivedTarget, TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> perceivingPlayer)
{
   check(perceivingPlayer.IsValid());
   for (int auraStateIndex = Items.Num() - 1; auraStateIndex >= 0; auraStateIndex--)
   {
      FTATAuraVisibilityState& auraState = Items[auraStateIndex];
      if (auraState.PerceivingPlayer == perceivingPlayer)
      {
         // Clear persistence timer before removing to avoid stale callbacks
         auraState.ClearPersistenceTimer(perceivedTarget);
         Items.RemoveAt(auraStateIndex);
      }
   }
}

ETATAuraVisibilityType FTATAuraVisibilityStateArray::ResolveLocalAuraVisibility() const
{
   ETATAuraVisibilityType highestAuraVisibility = ETATAuraVisibilityType::None;

   for (const FTATAuraVisibilityState& perceptionState : Items)
   {
      // Before evaluating aura visibility for the perceiving player, make sure the player has replicated! (otherwise we just wait for a subsequent call where the player's present)
      if (!perceptionState.PerceivingPlayer.IsValid())
      {
         continue;
      }

      if (perceptionState.IsPerceived())
      {
         // Lookup the associated perception sense on the perceiving player
         const FTATAuraVisibilityPerceiverSense* auraPerceiverSense = perceptionState.PerceivingPlayer->GetAuraVisibilityPerceiverSenses().FindByPredicate
         ([perceptionState](const FTATAuraVisibilityPerceiverSense& perceiverSettings)
            {
               return perceiverSettings.AuraSenseTag == perceptionState.AuraSenseTag;
            });

         check(auraPerceiverSense);
         const ETATAuraVisibilityType visibilityToCompare = perceptionState.PerceivingPlayer->GetOwnerCharacter()->IsLocallyControlled()
            ? auraPerceiverSense->PerceivingPlayerAuraVisibility : auraPerceiverSense->SharedAuraVisibility;

         // Pick the higher one and continue
         highestAuraVisibility = FMath::Max(highestAuraVisibility, visibilityToCompare);
      }
   }

   return highestAuraVisibility;
}

FTATAuraVisibilityState* FTATAuraVisibilityStateArray::_GetAuraVisibilityState(TWeakObjectPtr<const UTATAuraVisibilityPerceiverComponent> perceivingPlayer, FGameplayTag auraSenseTag)
{
   return Items.FindByPredicate([&](const FTATAuraVisibilityState& state) { return state.PerceivingPlayer == perceivingPlayer && state.AuraSenseTag == auraSenseTag; });
}
