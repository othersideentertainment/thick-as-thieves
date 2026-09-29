// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ose
#include "Abilities/OSEGameplayAbility.h"

// tat
#include "Abilities/HeightenedSenses/TATHeightenedSenseTypes.h"

// ue
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"

#include "TATGameplayAbility_HeightenedSenses.generated.h"

UCLASS()
class TAT_API UTATGameplayAbility_HeightenedSenses : public UOSEGameplayAbility
{
   GENERATED_BODY()

   UTATGameplayAbility_HeightenedSenses();

#if WITH_EDITOR
   // From UObject
   EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
   virtual void PostEditChangeChainProperty( struct FPropertyChangedChainEvent& propertyChangedEvent) override;
#endif // WITH_EDITOR

   // From UOSEGameplayAbility
   virtual void ActivateAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, const FGameplayEventData* triggerEventData) override;
   virtual void EndAbility(const FGameplayAbilitySpecHandle handle, const FGameplayAbilityActorInfo* actorInfo, const FGameplayAbilityActivationInfo activationInfo, bool replicateEndAbility, bool wasCancelled) override;

private:
   void _RefreshHeightenedSenseAkState();

   void _BindHeightenedSenseEvents(UAbilitySystemComponent* asc);
   void _UnbindHeightenedSenseEvents(UAbilitySystemComponent* asc);

   void _OnHeightenedSenseTagChanged(const FGameplayTag tag, int32 newCount);
   void _OnHeightenedSenseAttributeChanged(const FOnAttributeChangeData& data);
   void _OnHeightenedSenseGameplayEvent(FGameplayTag tag, const FGameplayEventData* payload);
   void _HandleChangeForHeightenedSense(const FTATHeightenedSensesEntry& heightenedSensesEntry);

   void _OnInitialDelayElapsed();
   void _OnDurationElapsed();

   void _UpdateHeightenedSenseTimers(const FTATHeightenedSensesEntry& senseEntry, FTATHeightenedSenseEventHistory& eventHistory);
   void _ClearHeightenedSenseTimers(FTATHeightenedSenseEventHistory& eventHistory);

   FTATHeightenedSenseEventHistory* _FindEventHistory(const FTATHeightenedSensesEntry& senseEntry, int32& outIndex);
   FTATHeightenedSenseEventHistory& _FindOrCreateEventHistory(const FTATHeightenedSensesEntry& senseEntry, int32& outIndex);
   FORCEINLINE FTATHeightenedSenseEventHistory* _FindEventHistory(const FTATHeightenedSensesEntry& senseEntry)
   {
      int32 outIndex = INDEX_NONE;
      return _FindEventHistory(senseEntry, outIndex);
   }
   FORCEINLINE FTATHeightenedSenseEventHistory& _FindOrCreateEventHistory(const FTATHeightenedSensesEntry& senseEntry)
   {
      int32 outIndex = INDEX_NONE;
      return _FindOrCreateEventHistory(senseEntry, outIndex);
   }
   void _RemoveEventHistoryAt(int32 index);

   FORCEINLINE const FTATHeightenedSensesEntry* _GetHeightenedSensesEntry(FGameplayTag tag) const 
   { 
      return _heightenedSenseEntries.FindByPredicate([tag](const FTATHeightenedSensesEntry& entry) { return entry.HasTag(tag); }); 
   }
   FORCEINLINE const FTATHeightenedSensesEntry* _GetHeightenedSensesEntry(const FGameplayAttribute& attribute) const 
   { 
      return _heightenedSenseEntries.FindByPredicate([attribute](const FTATHeightenedSensesEntry& entry) { return entry.HasAttribute(attribute); }); 
   }

   void _SetAkState(const UAkStateValue* akState);

   bool _MeetsTimeConstraints(const FTATHeightenedSensesEntry& heightenedSenseEntry);
   bool _MeetsTagRequirements(const FTATHeightenedSensesEntry& heightenedSenseEntry) const;

private:
   // Bookkeeping for querying FTATHeightenedSenseEntries ApplyAfterSeconds / DurationSeconds and binding refresh callbacks
   UPROPERTY()
   TArray<FTATHeightenedSenseEventHistory> _heightenedSenseEventHistoryEntries;

   // Order of entries define their priority (from high -> low)
   UPROPERTY(EditAnywhere)
   TArray<FTATHeightenedSensesEntry> _heightenedSenseEntries;

   // Default state to apply when no FTATHeightenedSenseEntries match against player state
   UPROPERTY(EditAnywhere)
   TObjectPtr<UAkStateValue> _defaultAkState = nullptr;
};
