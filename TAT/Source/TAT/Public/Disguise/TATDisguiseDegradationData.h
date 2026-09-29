// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Items/Disguise/TATDisguiseTool.h"
#include "Character/TATTeams.h"

// ue5
#include "AttributeSet.h"
#include "ScalableFloat.h"
#include "Engine/DataTable.h"

#include "TATDisguiseDegradationData.generated.h"

class UGameplayAbility;

UENUM(BlueprintType)
enum class ETATDisguiseVisibilityRequirement : uint8
{
   Always UMETA(Tooltip = "The reduction should happen whenever we receive the event"),

   /// The reduction should happen if we are being seen by an NPC that is not the one involved in the event (by checking Instigator and Target of the event payload)
   /// If we are unobserved, or only being seen by an NPC involved in the event, the reduction will not occur
   WhenSeenByUnrelatedNPC,

   WhenSeenByAnyNPC UMETA(Tooltip = "The reduction should happen if any NPC is observing us. If we are unobserved, the reduction will not occur"),
};

UENUM(BlueprintType)
enum class ETATDisguiseIntegrityEvent : uint8
{
   AlwaysReduceOverTime       UMETA(Tooltip = "Reduce integrity continuously regardless of state (acts essentially as a max duration)"),
   TagPresenceReduceOverTime  UMETA(Tooltip = "Reduce integrity continuously while a tag is present"),
   TagAdded                   UMETA(Tooltip = "Event occurs each time a tag is added to the character"),
   TagRemoved                 UMETA(Tooltip = "Event occurs each time a tag is removed from the character"),
   GameplayEvent              UMETA(Tooltip = "Gameplay event on the character"),
   AttributeDecrease          UMETA(Tooltip = "Attribute (health, stamina, etc.) decreased"),
   AbilityStarted             UMETA(Tooltip = "Character started using a gameplay ability"),
   NearbyCharacterState       UMETA(Tooltip = "The disguised character is too close to or being viewed by a nearby character"),
};

struct FTATDisguiseIntegrityResult
{
   bool InstantCancel = false;
   TOptional<float> IntegrityReduction;

   FTATDisguiseIntegrityResult() = default;
   explicit FTATDisguiseIntegrityResult(float integrityReduction) : InstantCancel(false), IntegrityReduction(integrityReduction) {}
   explicit FTATDisguiseIntegrityResult(bool instantCancel) : InstantCancel(instantCancel), IntegrityReduction(NullOpt) {}
   FTATDisguiseIntegrityResult(bool instantCancel, TOptional<float> integrityReduction) : InstantCancel(instantCancel), IntegrityReduction(integrityReduction) {}

   static FTATDisguiseIntegrityResult Combine(const FTATDisguiseIntegrityResult& lhs, const FTATDisguiseIntegrityResult& rhs, TFunctionRef<float(float, float)> op)
   {
      FTATDisguiseIntegrityResult result;
      result.InstantCancel = lhs.InstantCancel || rhs.InstantCancel;
      if (lhs.IntegrityReduction || rhs.IntegrityReduction)
      {
         result.IntegrityReduction = op(lhs.IntegrityReduction.Get(0.0f), rhs.IntegrityReduction.Get(0.0f));
      }
      return result;
   }

   FORCEINLINE explicit operator bool() const { return InstantCancel || (IntegrityReduction && IntegrityReduction.GetValue() > 0); }

   FTATDisguiseIntegrityResult operator+(const FTATDisguiseIntegrityResult& rhs) const { return Combine(*this, rhs, [](float a, float b) { return a + b; }); }
   FTATDisguiseIntegrityResult& operator+=(const FTATDisguiseIntegrityResult& rhs) { *this = Combine(*this, rhs, [](float a, float b) { return a + b; }); return *this; }

   FTATDisguiseIntegrityResult operator*(const FTATDisguiseIntegrityResult& rhs) const { return Combine(*this, rhs, [](float a, float b) { return a * b; }); }
   FTATDisguiseIntegrityResult& operator*=(const FTATDisguiseIntegrityResult& rhs) { *this = Combine(*this, rhs, [](float a, float b) { return a * b; }); return *this; }

   FORCEINLINE float ToDebugValue() const { return InstantCancel ? 99999.9f : IntegrityReduction.Get(0.0f); }
};

namespace DisguiseHelpers
{
template<typename T, uint32 NumInline = 3>
FString ArrayToString(const TArray<T>& arr)
{
   TArray<FString, TInlineAllocator<NumInline>> stringArr;
   stringArr.SetNum(arr.Num());
   for (int32 i = 0; i < arr.Num(); i++)
   {
      stringArr[i] = arr[i].ToString();
   }
   return FString::Join(stringArr, TEXT(", "));
}
} // namespace DisguiseHelpers


USTRUCT(BlueprintType)
struct FTATDisguiseIntegrityEventConfig_Attribute
{
   GENERATED_BODY()
   
   /// The event will trigger when this attribute is changed on the character
   UPROPERTY(EditAnywhere)
   FGameplayAttribute GameplayAttribute;
   
   /// The change in attribute values is multiplied by this value and added to IntegrityReduction (only when the result is greater than zero).
   /// This essentially works like this:
   ///   ActualIntegrityReduction = IntegrityReduction + Clamp((OldAttributeValue - NewAttributeValue) * AttributeDeltaMultiplier), 0, MaxExtraReductionFromAttributeDelta)
   /// Disabled if set to zero.
   /// If MaxExtraReductionFromAttributeDelta is zero, only the lower bounds is clamped.
   UPROPERTY(EditAnywhere)
   float AttributeDeltaMultiplier = 0.0f;

   /// The maximum amount that the integrity reduction can increase due to attribute changes.
   /// See the formula in the tooltip for AttributeDeltaMultiplier.
   /// Disabled if set to zero.
   UPROPERTY(EditAnywhere)
   float MaxExtraReductionFromAttributeDelta = 0.0f;
   
   FString GetDebugDescription() const
   {
      FString result = FString::Printf(TEXT("Attr: %s, Mult: %.2f"), *GameplayAttribute.AttributeName, AttributeDeltaMultiplier);
      if (MaxExtraReductionFromAttributeDelta != 0)
      {
         result += FString::Printf(TEXT("MaxReduction: %.2f"), MaxExtraReductionFromAttributeDelta);
      }
      return result;
   }
};


USTRUCT(BlueprintType)
struct FTATDisguiseIntegrityEventConfig_Ability
{
   GENERATED_BODY()
   
   /// Triggers for all activated abilities with ability tags (that is, tags on the ability itself) which match this query.
   /// If empty, it will trigger for _all_ activated abilities used by the character.
   UPROPERTY(EditAnywhere)
   FGameplayTagQuery AbilityTagQuery;

   /// Only apply to activated abilities?
   /// These are abilities that players explicitly activate by using an input (ex. pressing primary fire while holding the blackjack)
   /// This is defined as any ability where ActivationBlockedTags contains the "abilities disabled" gameplay tag.
   /// See also: [TAT] Project Settings -> DisableAbilitiesTag
   UPROPERTY(EditAnywhere)
   bool OnlyActivatedAbilities = false;

   /// If not empty, this row will only apply to abilities in this list.
   UPROPERTY(EditAnywhere)
   TArray<TSoftClassPtr<UGameplayAbility>> AbilityAllowList;

   /// If not empty, this row will never apply to abilities in this list.
   UPROPERTY(EditAnywhere)
   TArray<TSoftClassPtr<UGameplayAbility>> AbilityDenyList;
   
   FString GetDebugDescription() const
   {
      TArray<FString, TInlineAllocator<4>> parts;
      if (!AbilityTagQuery.IsEmpty()) { parts.Add(FString::Printf(TEXT("TagQuery: %s"), *AbilityTagQuery.GetDescription())); }
      if (OnlyActivatedAbilities) { parts.Add(TEXT("ActivatedOnly")); }
      if (!AbilityAllowList.IsEmpty()) { parts.Add(FString::Printf(TEXT("Allow: [%s]"), *DisguiseHelpers::ArrayToString(AbilityAllowList))); }
      if (!AbilityDenyList.IsEmpty()) { parts.Add(FString::Printf(TEXT("Deny: [%s]"), *DisguiseHelpers::ArrayToString(AbilityDenyList))); }
      return FString::Join(parts, TEXT(", ")); 
   }
};


UENUM(BlueprintType)
enum class ETATDisguiseIntegrityNearbyCharacterMode : uint8
{
   CharacterProximity         UMETA(DisplayName = "Character Proximity", Tooltip = "Character is within a certain radius of another character"),
   CharacterViewCone          UMETA(DisplayName = "Character View Cone", Tooltip = "Character is in the view cone of another character"),
   //CharacterInvestigating     UMETA(DisplayName = "Character Investigating", Tooltip = "An character is investigating the character (only works with NPCs)"),
};


USTRUCT(BlueprintType)
struct FTATDisguiseIntegrityEventConfig_Character
{
   GENERATED_BODY()
   
   UPROPERTY(EditAnywhere)
   ETATDisguiseIntegrityNearbyCharacterMode Mode = ETATDisguiseIntegrityNearbyCharacterMode::CharacterProximity;
   
   UPROPERTY(EditAnywhere)
   ETATTeamCharacterType CharacterType = ETATTeamCharacterType::Guard;

   UPROPERTY(EditAnywhere, meta = (EditCondition = "Mode == ETATDisguiseIntegrityNearbyCharacterMode::CharacterProximity || Mode == ETATDisguiseIntegrityNearbyCharacterMode::CharacterViewCone"))
   float CharacterRadius = 500.0f;

   UPROPERTY(EditAnywhere, meta = (EditCondition = "Mode == ETATDisguiseIntegrityNearbyCharacterMode::CharacterViewCone"))
   float CharacterViewConeAngleDegrees = 45.0f;
   
   /// If enabled, AI characters with limited sight (eg. in the dark) will not be able to see as far.
   /// If disabled, uses the full radius above.
   UPROPERTY(EditAnywhere, meta = (EditCondition = "Mode == ETATDisguiseIntegrityNearbyCharacterMode::CharacterViewCone"))
   bool UseAISightToLimitMaxViewConeDistance = false;

   FString GetDebugDescription() const
   {
      TArray<FString, TInlineAllocator<5>> parts;
      parts.Add(FString::Printf(TEXT("Mode: %s"), *StaticEnum<ETATDisguiseIntegrityNearbyCharacterMode>()->GetNameStringByValue(static_cast<int64>(Mode))));
      parts.Add(FString::Printf(TEXT("CharType: %s"), *StaticEnum<ETATTeamCharacterType>()->GetNameStringByValue(static_cast<int64>(CharacterType))));
      if (Mode == ETATDisguiseIntegrityNearbyCharacterMode::CharacterProximity || Mode == ETATDisguiseIntegrityNearbyCharacterMode::CharacterViewCone)
      {
         parts.Add(FString::Printf(TEXT("Radius: %.2f"), CharacterRadius));
      }
      if (Mode == ETATDisguiseIntegrityNearbyCharacterMode::CharacterViewCone)
      {
         parts.Add(FString::Printf(TEXT("ViewConeAngle: %.2f deg"), CharacterViewConeAngleDegrees));
         if (UseAISightToLimitMaxViewConeDistance) { parts.Add(TEXT("UseAISightToLimitMaxViewConeDistance")); }
      }
      return FString::Join(parts, TEXT(", "));
   }
};


USTRUCT(BlueprintType)
struct FTATDisguiseIntegrityReductionDataRow : public FTableRowBase
{
   GENERATED_BODY()

public:
   /// Can uncheck this to disable rows without deleting them
   UPROPERTY(EditAnywhere)
   bool Enabled = true;

   /// When to trigger this disguise integrity event
   UPROPERTY(EditAnywhere)
   ETATDisguiseIntegrityEvent EventType = ETATDisguiseIntegrityEvent::TagPresenceReduceOverTime;

   /// The event is only used when the player's stealth score is in this range
   UPROPERTY(EditAnywhere)
   FFloatRange StealthScoreRange = FFloatRange(FFloatRangeBound::Open(), FFloatRangeBound::Open());

   /// The event will trigger when the character's ability system component has this (exact) gameplay tag
   UPROPERTY(EditAnywhere, meta = (EditCondition = "EventType == ETATDisguiseIntegrityEvent::TagPresenceReduceOverTime || EventType == ETATDisguiseIntegrityEvent::TagAdded || EventType == ETATDisguiseIntegrityEvent::TagRemoved", EditConditionHides))
   FGameplayTag GameplayTag;

   /// The event will trigger when a gameplay event with this tag is activated
   UPROPERTY(EditAnywhere, meta = (EditCondition = "EventType == ETATDisguiseIntegrityEvent::GameplayEvent", EditConditionHides))
   FGameplayTag GameplayEvent;
   
   /// If this degradation requires being seen by an NPC, an NPC not involved in the reduction event, or happens always
   UPROPERTY(EditAnywhere, meta = (EditCondition = "EventType == ETATDisguiseIntegrityEvent::GameplayEvent"))
   ETATDisguiseVisibilityRequirement VisibilityRequirement = ETATDisguiseVisibilityRequirement::Always;

   /// Attribute decrease event settings
   UPROPERTY(EditAnywhere, meta = (EditCondition = "EventType == ETATDisguiseIntegrityEvent::AttributeDecrease", EditConditionHides))
   FTATDisguiseIntegrityEventConfig_Attribute AttributeDecrease;

   /// Ability started event settings
   UPROPERTY(EditAnywhere, meta = (EditCondition = "EventType == ETATDisguiseIntegrityEvent::AbilityStarted", EditConditionHides))
   FTATDisguiseIntegrityEventConfig_Ability AbilityStarted;

   /// Nearby character event settings
   UPROPERTY(EditAnywhere, meta = (EditCondition = "EventType == ETATDisguiseIntegrityEvent::NearbyCharacterState", EditConditionHides))
   FTATDisguiseIntegrityEventConfig_Character NearbyCharacterState;

   /// Should this event cause the disguise to break immediately?
   UPROPERTY(EditAnywhere, meta = (EditCondition = "EventType != ETATDisguiseIntegrityEvent::AlwaysReduceOverTime && EventType != ETATDisguiseIntegrityEvent::TagPresenceReduceOverTime"))
   bool InstantCancel = false;

   /// How much to reduce disguise integrity.
   /// If EventType is set to TagPresenceReduceOverTime, this is the amount to reduce per second.
   /// For all other event types, the reduction occurs once per event.
   /// The curve will be sampled at the upgrade level for IntegrityReductionUpgradeTag, or zero if no upgrade tag is specified or the character does not have that upgrade.
   UPROPERTY(EditAnywhere, meta = (EditCondition = "!InstantCancel || (EventType != ETATDisguiseIntegrityEvent::AlwaysReduceOverTime && EventType != ETATDisguiseIntegrityEvent::TagPresenceReduceOverTime)"))
   FScalableFloat IntegrityReduction = 0.0f;

   /// If specified, the IntegrityReduction curve will be sampled at the upgrade level (or zero if the character does not have the upgrade).
   UPROPERTY(EditAnywhere, meta = (EditCondition = "!InstantCancel || (EventType != ETATDisguiseIntegrityEvent::AlwaysReduceOverTime && EventType != ETATDisguiseIntegrityEvent::TagPresenceReduceOverTime)", Categories = "Upgrade"))
   FGameplayTag IntegrityReductionUpgradeTag;
   
   FORCEINLINE bool IsReduceOverTimeEvent() const
   {
      return EventType == ETATDisguiseIntegrityEvent::AlwaysReduceOverTime || EventType == ETATDisguiseIntegrityEvent::TagPresenceReduceOverTime;
   }
   
   FORCEINLINE bool IsInstantCancelEvent() const
   {
      return IsReduceOverTimeEvent() ? false : InstantCancel;
   }

   FString GetIntegrityReductionDebugValue() const
   {
      if (IsInstantCancelEvent())
      {
         return TEXT("Instant Cancel");
      }
      return IsReduceOverTimeEvent()
         ? FString::Printf(TEXT("%.2f / sec"), IntegrityReduction.Value)
         : FString::Printf(TEXT("%.2f"), IntegrityReduction.Value);
   }
   
   FString GetDebugDescription() const
   {
      switch (EventType)
      {
      case ETATDisguiseIntegrityEvent::AlwaysReduceOverTime:
         return FString();
      case ETATDisguiseIntegrityEvent::TagPresenceReduceOverTime:
         // fallthrough
      case ETATDisguiseIntegrityEvent::TagAdded:
         // fallthrough
      case ETATDisguiseIntegrityEvent::TagRemoved:
         return FString::Printf(TEXT("Tag: %s"), *GameplayTag.ToString());
      case ETATDisguiseIntegrityEvent::GameplayEvent:
         return FString::Printf(TEXT("Event: %s"), *GameplayEvent.ToString());
      case ETATDisguiseIntegrityEvent::AttributeDecrease:
         return AttributeDecrease.GetDebugDescription();
      case ETATDisguiseIntegrityEvent::AbilityStarted:
         return AbilityStarted.GetDebugDescription();
      case ETATDisguiseIntegrityEvent::NearbyCharacterState:
         return NearbyCharacterState.GetDebugDescription();
      default:
         break;
      }
      return TEXT("INVALID");
   }
};

