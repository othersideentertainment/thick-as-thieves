// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue5
#include "Engine/DataAsset.h"
#include "GameplayEffect.h"

#include "OSEGameplayEffectSet.generated.h"


class IAbilitySystemInterface;
class UAbilitySystemComponent;
class UOSEAbilitySystemComponent;


USTRUCT()
struct OSECORE_API FOSEGameplayEffectSetStackCount
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   FScalableFloat StackCount;

   /// The upgrade tag to index into the StackCount curve (or none)
   UPROPERTY(EditDefaultsOnly, Meta = (Categories = "Upgrade"))
   FGameplayTag StackCountUpgradeTag;

   int32 Calculate(const UOSEAbilitySystemComponent* asc) const;
};

USTRUCT()
struct OSECORE_API FOSEGameplayEffectSetRequirements
{
   GENERATED_BODY()

   /// Tag to use for upgrade-related requirements
   UPROPERTY(EditDefaultsOnly)
   FGameplayTag UpgradeTag;

   /// If UpgradeTagRequired is enabled, this is the minimum upgrade level required for the effect to apply
   UPROPERTY(EditDefaultsOnly, Meta = (UIMin = 1, ClampMin = 1))
   int32 RequiredUpgradeLevel = 1;

   /// If enabled, the gameplay effect will be created at the level of the upgrade tag (or 0 if the upgrade is not present)
   UPROPERTY(EditDefaultsOnly)
   bool UseUpgradeLevelAsEffectLevel = true;
};

USTRUCT()
struct OSECORE_API FOSEGameplayEffectSetEntry
{
   GENERATED_BODY()

public:

   UPROPERTY(EditDefaultsOnly)
   TSubclassOf<UGameplayEffect> Effect;

   UPROPERTY(EditDefaultsOnly, meta = (InlineEditConditionToggle))
   bool UseCustomStackCount = false;

   UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "UseCustomStackCount"))
   FOSEGameplayEffectSetStackCount StackCount;

   UPROPERTY(EditDefaultsOnly, Meta = (InlineEditConditionToggle))
   bool UseRequirements = false;

   UPROPERTY(EditDefaultsOnly, Meta = (EditCondition = "UseRequirements"))
   FOSEGameplayEffectSetRequirements Requirements;
};

/// A set of effects that can be applied at once
UCLASS(ClassGroup = (Ability))
class OSECORE_API UOSEGameplayEffectSet : public UDataAsset
{
   GENERATED_BODY()

public:

   virtual TArray<FActiveGameplayEffectHandle> ApplyEffects(IAbilitySystemInterface* abilitySystemInterface) const;
   virtual TArray<FActiveGameplayEffectHandle> ApplyEffects(UAbilitySystemComponent* abilitySystemComponent) const;

#if WITH_EDITOR
   // UObject
   virtual void PostLoad() override;
#endif
protected:

#if WITH_EDITORONLY_DATA
   UPROPERTY()
   TArray<TSubclassOf<UGameplayEffect>> Effects_DEPRECATED;
#endif

   UPROPERTY(EditDefaultsOnly, Category = "Ability", meta = (DisplayName="Effects", TitleProperty="Effect"))
   TArray<FOSEGameplayEffectSetEntry> EffectEntries;
};
