// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Items/ToolAbility.h"

// ue
#include "AbilitySystemComponent.h"

// ose
#include "Items/ToolComponent.h"
#include "Items/ToolInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ToolAbility)

DEFINE_LOG_CATEGORY_STATIC(LogOSEToolAbility, Log, All);

// Sets default values
UToolAbility::UToolAbility()
   : Super()
{
   ActivationRequiresController = true;
   NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
   InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

// Returns true if this ability can be activated right now. Has no side effects
bool UToolAbility::CanActivateAbility(
   const FGameplayAbilitySpecHandle handle,
   const FGameplayAbilityActorInfo* actorInfo,
   const FGameplayTagContainer* sourceTags /* = nullptr */,
   const FGameplayTagContainer* targetTags /* = nullptr */,
   OUT FGameplayTagContainer* optionalRelevantTags /* = nullptr */) const
{
   // Call base class
   if (!Super::CanActivateAbility(handle, actorInfo, sourceTags, targetTags, optionalRelevantTags))
      return false;

   // Make sure the ability system component is valid
   UAbilitySystemComponent* const abilitySystemComponent = actorInfo->AbilitySystemComponent.Get();
   if (!abilitySystemComponent)
      return false;

   // Grab the spec from our handle
   FGameplayAbilitySpec* spec = abilitySystemComponent->FindAbilitySpecFromHandle(handle);
   if (!spec)
      return false;

   // Get our source object
   const UObject* sourceObject = spec->SourceObject.Get();
   if (!sourceObject)
      return false;

   // Our source object must implement IToolInterface
   const IToolInterface* sourceTool = Cast<IToolInterface>(sourceObject);
   if (sourceTool == nullptr)
      return false;

   if (AllowUnequippedActivation)
   {
      return sourceTool->IsReady();
   }

   // We can only activate the ability if our associated tool is active
   return sourceTool->IsEquipped();
}

TScriptInterface<IToolInterface> UToolAbility::GetToolInterface() const
{
   return GetCurrentSourceObject();
}

bool UToolAbility::IsToolReady() const
{
   if (auto tool = GetToolInterface())
      return tool->IsReady();

   return false;
}

bool UToolAbility::IsToolEquipped() const
{
   if (auto tool = GetToolInterface())
      return tool->IsEquipped();

   return false;
}

UToolComponent* UToolAbility::GetSourceToolForAbility(const UGameplayAbility* ability)
{
   if (!IsValid(ability))
   {
      UE_LOG(LogOSEToolAbility, Warning, TEXT("Attempting to call GetSourceToolForAbility() on a null Ability!"));
      return nullptr;
   }
   // Source object only exists for instanced abilities
   if (!ability->IsInstantiated())
   {
      return nullptr;
   }

   // Tool abilities (and proxy-abilities) always have the UToolComponent as the source object
   return Cast<UToolComponent>(ability->GetCurrentSourceObject());
}

