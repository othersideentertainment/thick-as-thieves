// (c) 2021-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// TAT
#include "Animation/TATAnimData.h"
#include "Animation/TATAnimSettings.h"

// OSE
#include "Animation/Graph/OSEAnimActorInfo.h"

// UE4
#include "GameplayTagAssetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAnimData)


//--------------------------------------------------------------------------------------------------
// Updates ability data
//--------------------------------------------------------------------------------------------------

FTATAnimAbilityData::FTATAnimAbilityData()
   : FOSEBaseAnimData()
     , IsDowned(false)
     , IsUnconscious(false)
     , IsCowering(false)
     , IsBeingCarried(false)
     , IsCarrying(false)
     , IsBeingThrown(false)
     , IsMageHandCasting(false)
     , IsMageHandExtended(false)
     , IsMageHandInTimedInteraction(false)
     , IsWireAttached(false)
     , IsWireInGrappleMode(false)
     , IsWireInRappelMode(false)
     , IsInMelee(false)
     , IsInLightMelee(false)
     , IsInHeavyMelee(false)
     , IsBlocking(false)
     , IsTakingDownOverhead(false)
     , IsNervous(false)
     , IsLocalPlayer(false)
{
}

void FTATAnimAbilityData::Update(const FOSEAnimActorInfo& actorInfo)
{
   TRACE_CPUPROFILER_EVENT_SCOPE(FTATAnimAbilityData::Update)
   const FTATAnimAbilityTags& abilityTags = UTATAnimSettings::Get().GetAbilityTags();
   Update(actorInfo.GetGameplayTagInterface(), abilityTags, actorInfo.IsLocallyControlled);

   // Additional checks to make sure rappelling mode is detected properly
   // (see original ABP_Default_Hotwire comment in UpdateHotwireVariables function)
   if (IsWireAttached && !IsWireInRappelMode)
   {
      IsWireInRappelMode = actorInfo.IsAffectedByDistanceConstraint && (actorInfo.MovementMode == EMovementMode::MOVE_Falling);
   }

   IsLocalPlayer = actorInfo.IsLocallyControlled && actorInfo.PawnOwner->IsPlayerControlled();
}

void FTATAnimAbilityData::Update(
   const IGameplayTagAssetInterface* tagInterface,
   const FTATAnimAbilityTags& abilityTags,
   bool isLocallyControlled)
{
   if (tagInterface != nullptr)
   {
      IsUnconscious = tagInterface->HasAnyMatchingGameplayTags(abilityTags.ConditionUnconsciousTags);
      IsDowned = tagInterface->HasAnyMatchingGameplayTags(abilityTags.ConditionDownedTags);
      
      IsCowering = tagInterface->HasAnyMatchingGameplayTags(abilityTags.StatusCoweringTags);
      IsNervous = tagInterface->HasAnyMatchingGameplayTags(abilityTags.NervousTags); 
      
      IsBeingCarried = tagInterface->HasAnyMatchingGameplayTags(abilityTags.CarriedTags);
      IsCarrying = tagInterface->HasAnyMatchingGameplayTags(abilityTags.CarryingTags);
      IsBeingThrown = tagInterface->HasAnyMatchingGameplayTags(abilityTags.ThrownTags);
      
      IsMageHandCasting = tagInterface->HasAnyMatchingGameplayTags(abilityTags.MageHandCastingTags);
      IsMageHandExtended = tagInterface->HasAnyMatchingGameplayTags(abilityTags.MageHandExtendedTags);
      IsMageHandInTimedInteraction = tagInterface->HasAnyMatchingGameplayTags(abilityTags.MageHandTimedInteractionTags);

      IsWireAttached = tagInterface->HasAnyMatchingGameplayTags(abilityTags.WireAttachedTags);
      IsWireInGrappleMode = tagInterface->HasAnyMatchingGameplayTags(abilityTags.WireModeGrappleTags);
      IsWireInRappelMode = tagInterface->HasAnyMatchingGameplayTags(abilityTags.WireModeRappelTags);

      IsInLightMelee = tagInterface->HasAnyMatchingGameplayTags(abilityTags.MeleeLightTags);
      IsInHeavyMelee = tagInterface->HasAnyMatchingGameplayTags(abilityTags.MeleeHeavyTags);
      IsInMelee = (IsInLightMelee || IsInHeavyMelee) || tagInterface->HasAnyMatchingGameplayTags(abilityTags.MeleeTags);
      IsBlocking = tagInterface->HasAnyMatchingGameplayTags(isLocallyControlled ? abilityTags.BlockLocalTags : abilityTags.BlockRemoteTags);
      IsTakingDownOverhead = tagInterface->HasAnyMatchingGameplayTags(abilityTags.TakingDownOverheadTags);
   }
}

