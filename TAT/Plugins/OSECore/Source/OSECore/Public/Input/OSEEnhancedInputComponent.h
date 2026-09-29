// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose

// ue4
#include "EnhancedInputComponent.h"

#include "OSEEnhancedInputComponent.generated.h"

class UEnhancedAbilityInputActionsAsset;
class UInputMappingContext;
class UOSECharacterInputActionsAsset;

UCLASS(BlueprintType)
class OSECORE_API UOSEEnhancedInputComponent : public UEnhancedInputComponent
{
   GENERATED_BODY()

public:
   UOSEEnhancedInputComponent();

   // from UActorComponent
   virtual void InitializeComponent() override;

private:
   UPROPERTY(Transient)
   UOSECharacterInputActionsAsset* _loadedInputActions = nullptr;
   UPROPERTY(Transient)
   TArray<UInputMappingContext*> _loadingInputMappingContexts;
   UPROPERTY(Transient)
   UEnhancedAbilityInputActionsAsset* _loadedEnhancedAbilityInputActionsAsset = nullptr;
};
