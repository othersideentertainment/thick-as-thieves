// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Input/OSEEnhancedInputComponent.h"

// ose
#include "Abilities/OSEAbilityInputBinds.h"
#include "Character/OSECharacterUtils.h"
#include "Input/OSEEnhancedInputPriority.h"
#include "Input/OSEInputSettings.h"

// ue5
#include "InputMappingContext.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEEnhancedInputComponent)

// ue4

UOSEEnhancedInputComponent::UOSEEnhancedInputComponent()
   : Super()
{
   bWantsInitializeComponent = true;
}

void UOSEEnhancedInputComponent::InitializeComponent()
{
   Super::InitializeComponent();

   // load and hold onto refs to our project-level imc's and
   // action mappings so other classes that use them don't have to load them
   const UOSEInputDeveloperSettings& inputSettings = UOSEInputDeveloperSettings::Get();
   _loadedInputActions = inputSettings.DefaultCharacterInputActionsAsset.LoadSynchronous();
   _loadedEnhancedAbilityInputActionsAsset = inputSettings.DefaultEnhancedAbilityInputActionsAsset.LoadSynchronous();
   for (const FOSEInputContextPriority& context : inputSettings.DefaultCharacterInputMappingContexts)
   {
      _loadingInputMappingContexts.AddUnique(context.Context.LoadSynchronous());
   }
}

