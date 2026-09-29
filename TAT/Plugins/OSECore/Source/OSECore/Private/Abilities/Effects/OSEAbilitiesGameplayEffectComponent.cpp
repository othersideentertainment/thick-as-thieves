// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "Abilities/Effects/OSEAbilitiesGameplayEffectComponent.h"

// ose
#include "Abilities/OSEAbilitySystemComponent.h"

// ue
#include "AbilitySystemComponent.h"
#include "AbilitySystemLog.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAbilitiesGameplayEffectComponent)

#define LOCTEXT_NAMESPACE "OSEAbilitiesGameplayEffectComponent"

UOSEAbilitiesGameplayEffectComponent::UOSEAbilitiesGameplayEffectComponent()
{
#if WITH_EDITORONLY_DATA
	EditorFriendlyName = TEXT("[OSE] Grant Abilities While Active");
#endif
}

// Reminder: this is happening on the authority only
void UOSEAbilitiesGameplayEffectComponent::OnInhibitionChanged(FActiveGameplayEffectHandle ActiveGEHandle, bool bIsInhibited) const
{
	if (bIsInhibited)
	{
		RemoveAbilities(ActiveGEHandle);
	}
	else
	{
		GrantAbilities(ActiveGEHandle);
	}
}

// Reminder: this is happening on the authority only
void UOSEAbilitiesGameplayEffectComponent::GrantAbilities(FActiveGameplayEffectHandle ActiveGEHandle) const
{
	UOSEAbilitySystemComponent* ASC = Cast<UOSEAbilitySystemComponent>(ActiveGEHandle.GetOwningAbilitySystemComponent());
	if (!ensure(ASC))
	{
		UE_LOG(LogGameplayEffects, Error, TEXT("%s was passed an ActiveGEHandle %s which did not have a valid associated AbilitySystemComponent"), ANSI_TO_TCHAR(__func__), *ActiveGEHandle.ToString());
		return;
	}

	if (ASC->bSuppressGrantAbility)
	{
		UE_LOG(LogGameplayEffects, Warning, TEXT("%s suppressed by %s bSuppressGrantAbility"), *GetName(), *ASC->GetName());
		return;
	}

	const FActiveGameplayEffect* ActiveGE = ASC->GetActiveGameplayEffect(ActiveGEHandle);
	if (!ActiveGE)
	{
		UE_LOG(LogGameplayEffects, Warning, TEXT("ActiveGEHandle %s did not corresponds to Active Gameplay Effect on %s. This could potentially happen if you remove the GE during the application of other GE's"), *ActiveGEHandle.ToString(), *ASC->GetName());
		return;
	}
	const FGameplayEffectSpec& ActiveGESpec = ActiveGE->Spec;

	const TArray<FGameplayAbilitySpec>& AllAbilities = ASC->GetActivatableAbilities();
	for (const FOSEGameplayEffectAbilityGrantConfig& AbilityConfig : _grantAbilityConfigs)
	{
		// Check that we're configured
		const UGameplayAbility* AbilityCDO = AbilityConfig.AbilityClass.GetDefaultObject();
		if (!AbilityCDO)
		{
			continue;
		}

		if(!AbilityConfig.Requirements.IsMet(ASC))
		{
			continue;
		}

		// Only do this if we haven't assigned the ability yet! This prevents cases where stacking GEs
		// would regrant the ability every time the stack was applied
		const bool bAlreadyGrantedAbility = AllAbilities.ContainsByPredicate([ASC, AbilityCDO, &ActiveGEHandle](FGameplayAbilitySpec& Spec) { return Spec.Ability == AbilityCDO && Spec.GameplayEffectHandle == ActiveGEHandle; });
		if (bAlreadyGrantedAbility)
		{
			continue;
		}

		const int32 level = static_cast<int32>(AbilityConfig.LevelScalableFloat.GetValueAtLevel(ActiveGESpec.GetLevel()));
		const int32 inputId = AbilityConfig.bBindToInput ? static_cast<int32>(AbilityConfig.InputCommand) : -1;

		// Now grant that ability to the owning actor
		FGameplayAbilitySpec AbilitySpec{ AbilityConfig.AbilityClass, level, inputId, ActiveGESpec.GetEffectContext().GetSourceObject() };
		AbilitySpec.SetByCallerTagMagnitudes = ActiveGESpec.SetByCallerTagMagnitudes;
		AbilitySpec.GameplayEffectHandle = ActiveGEHandle;

		ASC->GiveAbility(AbilitySpec);
	}
}

// Reminder: this is happening on the authority only and the ActiveGEHandle can be considered 'inactive' by this time.
void UOSEAbilitiesGameplayEffectComponent::RemoveAbilities(FActiveGameplayEffectHandle ActiveGEHandle) const
{
	UAbilitySystemComponent* ASC = ActiveGEHandle.GetOwningAbilitySystemComponent();
	if (!ensure(ASC))
	{
		UE_LOG(LogGameplayEffects, Error, TEXT("%s was passed an ActiveGEHandle %s which did not have a valid associated AbilitySystemComponent"), ANSI_TO_TCHAR(__func__), *ActiveGEHandle.ToString());
		return;
	}

	FScopedAbilityListLock ScopedAbilityListLock(*ASC);
	const TArray<const FGameplayAbilitySpec*> GrantedAbilities = ASC->FindAbilitySpecsFromGEHandle(ScopedAbilityListLock, ActiveGEHandle, EConsiderPending::All);
	for (const FOSEGameplayEffectAbilityGrantConfig& AbilityConfig : _grantAbilityConfigs)
	{
		// Check that we're configured
		const UGameplayAbility* AbilityCDO = AbilityConfig.AbilityClass.GetDefaultObject();
		if (!AbilityCDO)
		{
			continue;
		}

		// See if we were granted, and if so we can remove it
		const FGameplayAbilitySpec* const* AbilitySpecItem = GrantedAbilities.FindByPredicate([AbilityCDO](const FGameplayAbilitySpec* Spec) { return Spec->Ability == AbilityCDO; });
		if (!AbilitySpecItem || !(*AbilitySpecItem))
		{
			continue;
		}
		const FGameplayAbilitySpec& AbilitySpecDef = (**AbilitySpecItem);

		switch (AbilityConfig.RemovalPolicy)
		{
			case EGameplayEffectGrantedAbilityRemovePolicy::CancelAbilityImmediately:
			{
				ASC->ClearAbility(AbilitySpecDef.Handle);
				break;
			}
			case EGameplayEffectGrantedAbilityRemovePolicy::RemoveAbilityOnEnd:
			{
				ASC->SetRemoveAbilityOnEnd(AbilitySpecDef.Handle);
				break;
			}
			default:
			{
				// Do nothing to granted ability
				break;
			}
		}
	}
}

bool UOSEAbilitiesGameplayEffectComponent::OnActiveGameplayEffectAdded(FActiveGameplayEffectsContainer& ActiveGEContainer, FActiveGameplayEffect& ActiveGE) const
{
	if (ActiveGEContainer.IsNetAuthority())
	{
		ActiveGE.EventSet.OnEffectRemoved.AddUObject(this, &ThisClass::OnActiveGameplayEffectRemoved);
		ActiveGE.EventSet.OnInhibitionChanged.AddUObject(this, &ThisClass::OnInhibitionChanged);
	}

	return true;
}

void UOSEAbilitiesGameplayEffectComponent::OnActiveGameplayEffectRemoved(const FGameplayEffectRemovalInfo& RemovalInfo) const
{
	const FActiveGameplayEffect* ActiveGE = RemovalInfo.ActiveEffect;
	if (!ensure(ActiveGE))
	{
		UE_LOG(LogGameplayEffects, Error, TEXT("FGameplayEffectRemovalInfo::ActiveEffect was not populated in %s"), ANSI_TO_TCHAR(__func__));
		return;
	}

	RemoveAbilities(ActiveGE->Handle);
}

#if WITH_EDITOR
EDataValidationResult UOSEAbilitiesGameplayEffectComponent::IsDataValid(FDataValidationContext& Context) const
{
	EDataValidationResult Result = Super::IsDataValid(Context);

	if (GetOwner()->DurationPolicy == EGameplayEffectDurationType::Instant)
	{
		if (_grantAbilityConfigs.Num() > 0)
		{
			Context.AddError(FText::FormatOrdered(LOCTEXT("InstantDoesNotWorkWithGrantAbilities", "GrantAbilityConfigs does not work with Instant Effects: {0}."), FText::FromString(GetClass()->GetName())));
			Result = EDataValidationResult::Invalid;
		}
	}

	for (int Index = 0; Index < _grantAbilityConfigs.Num(); ++Index)
	{
		const TSubclassOf<UGameplayAbility> AbilityClass = _grantAbilityConfigs[Index].AbilityClass;
		for (int CheckIndex = Index + 1; CheckIndex < _grantAbilityConfigs.Num(); ++CheckIndex)
		{
			if (AbilityClass == _grantAbilityConfigs[CheckIndex].AbilityClass)
			{
				Context.AddError(FText::FormatOrdered(LOCTEXT("GrantAbilitiesMustBeUnique", "Multiple Abilities of the same type cannot be granted by {0}."), FText::FromString(GetClass()->GetName())));
				Result = EDataValidationResult::Invalid;
			}
		}
	}

	return Result;
}
#endif // WITH_EDITOR

#undef LOCTEXT_NAMESPACE
