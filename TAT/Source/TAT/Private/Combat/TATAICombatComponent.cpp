// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Combat/TATAICombatComponent.h"

// ose
#include "Character/OSECharacterBase.h"

// ue
#include "AbilitySystemComponent.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAICombatComponent)

//---------------------------------------------------------------------------------------
// UTATAICombatCombosAsset
//---------------------------------------------------------------------------------------

const FGameplayTag* UTATAICombatCombosAsset::FindTagAtIndex(int comboIndex, int abilityIndex)
{
   if (Combos.IsValidIndex(comboIndex))
   {
      const FTATAICombatCombo& combo = Combos[comboIndex];
      if (combo.AbilityTags.IsValidIndex(abilityIndex))
      {
         return &combo.AbilityTags[abilityIndex];
      }
   }
   return nullptr;
}

#if WITH_EDITOR
EDataValidationResult UTATAICombatCombosAsset::IsDataValid(FDataValidationContext& context) const
{
   if (Combos.Num() == 0)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no combos!"), *GetName())));
   }
   for (int idx = 0; idx < Combos.Num(); ++idx)
   {
      const FTATAICombatCombo& combo = Combos[idx];
      if (combo.AbilityTags.Num() == 0)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] combo entry at %d has no ability tags!"), *GetName(), idx)));
      }

      for(const FGameplayTag& abilityTag : combo.AbilityTags)
      {
         if (!abilityTag.IsValid())
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("[%s] combo entry at %d has invalid ability tag %s!"), *GetName(), idx, *abilityTag.ToString())));
         }
      }
   }
   return context.GetNumErrors() + context.GetNumWarnings() ? EDataValidationResult::Invalid : EDataValidationResult::Valid;
}
#endif // WITH_EDITOR

//---------------------------------------------------------------------------------------
// UTATAICombatComponent
//---------------------------------------------------------------------------------------

UTATAICombatComponent::UTATAICombatComponent()
   : Super()
{
}

void UTATAICombatComponent::BeginPlay()
{
   Super::BeginPlay();

   if (GetOwner()->HasAuthority() && ComboAsset)
   {
      if (AOSECharacterBase* ownerCharacter = Cast<AOSECharacterBase>(GetOwner()))
      {
         if (UAbilitySystemComponent* asc = ownerCharacter->GetAbilitySystemComponent())
         {
            _onAbilityActivatedDelegateHandle = asc->AbilityActivatedCallbacks.AddUObject(this, &UTATAICombatComponent::_OnAbilityActivated);
         }
      }

      _ChooseNewCombo();
   }
}

void UTATAICombatComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (GetOwner()->HasAuthority() && ComboAsset)
   {
      if (AOSECharacterBase* ownerCharacter = Cast<AOSECharacterBase>(GetOwner()))
      {
         if (UAbilitySystemComponent* asc = ownerCharacter->GetAbilitySystemComponent())
         {
            asc->AbilityActivatedCallbacks.Remove(_onAbilityActivatedDelegateHandle);
         }
      }
   }

   Super::EndPlay(endPlayReason);
}

FGameplayTag UTATAICombatComponent::FindCurrentComboAbilityTag() const
{
   if (ComboAsset)
   {
      if (const FGameplayTag* abilityTag = ComboAsset->FindTagAtIndex(_currentComboIdx, _currentAbilityIdx))
      {
         return *abilityTag;
      }
   }
   return FGameplayTag::EmptyTag;
}

void UTATAICombatComponent::ResetCombo()
{
   // ASSUMPTION: if we're at the 0th ability in a combo we're already setup to start a new combo, so do nothing
   if (_currentAbilityIdx != 0)
   {
      _ChooseNewCombo();
   }
}

void UTATAICombatComponent::_OnAbilityActivated(UGameplayAbility* ability)
{
   // assume we don't get passed null...?
   check(ability);

   // assume we only bound if we had an asset...?
   check(ComboAsset);

   // did we execute our next combo ability?
   bool comboAbilityActivated = false;
   const FGameplayTagContainer& abilityTags = ability->GetAssetTags();
   const FGameplayTag* waitingForComboAbilityTag = ComboAsset->FindTagAtIndex(_currentComboIdx, _currentAbilityIdx);
   if (waitingForComboAbilityTag && abilityTags.HasTag(*waitingForComboAbilityTag))
   {
      comboAbilityActivated = true;
   }

   if (comboAbilityActivated)
   {
      UE_LOG(LogCombatComponent, Verbose, TEXT("%s combo ability activated (%s)"), *GetName(), *ability->GetName());

      // did not drop our combo so pick the next move in the combo (which may trigger a new combo)
      _UpdateNextComboAbility();
   }

   // TODO: Need to drop the combo off once the player attacks us back and we get into that type of behavior
}

void UTATAICombatComponent::_UpdateNextComboAbility()
{
   check(GetOwner()->HasAuthority());
   check(ComboAsset);

   if (ComboAsset->Combos.IsValidIndex(_currentComboIdx))
   {
      // next ability in the combo
      ++_currentAbilityIdx;

      // if it's outside of the range, pick a new combo
      const FTATAICombatCombo& combo = ComboAsset->Combos[_currentComboIdx];
      if (combo.AbilityTags.IsValidIndex(_currentAbilityIdx))
      {
         // valid ability, do nothing
         UE_LOG(LogCombatComponent, Verbose, TEXT("%s waiting for next combo ability with tag %s"), *GetName(), *combo.AbilityTags[_currentAbilityIdx].ToString());
      }
      else
      {
         UE_LOG(LogCombatComponent, Verbose, TEXT("%s invalid ability index %d, picking a new combo!"), *GetName(), _currentAbilityIdx);

         // invalid ability, pick a new combo
         _ChooseNewCombo();
      }
   }
}

void UTATAICombatComponent::_ChooseNewCombo()
{
   check(GetOwner()->HasAuthority());
   check(ComboAsset);
   
   if (ComboAsset->Combos.Num() > 0)
   {
      _currentComboIdx = FMath::RandHelper(ComboAsset->Combos.Num());
      _currentAbilityIdx = 0; // always start at the beginning of the combo
      UE_LOG(LogCombatComponent, Verbose, TEXT("%s combo \"%s\" chosen"), *GetName(), *ComboAsset->Combos[_currentComboIdx].ComboName.ToString());
   }
   else
   {
      _currentComboIdx = INDEX_NONE;
      _currentAbilityIdx = INDEX_NONE;
   }
}

