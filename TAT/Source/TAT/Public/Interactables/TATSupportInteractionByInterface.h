// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue
#include "GameplayTagContainer.h"
#include "UObject/Interface.h"

#include "TATSupportInteractionByInterface.generated.h"

// This class does not need to be modified.
UINTERFACE(Blueprintable, MinimalAPI, Category = "Interactable")
class UTATSupportInteractionByInterface : public UInterface
{
   GENERATED_BODY()
};

/// This interface can be applied to interactables that support being interacted with by more than just player characters
/// The default behavior for an interactable is that the player character can interact with it, but not others (e.g. astral projection)
/// This interface allows us to support non-player interactors by overriding DoesSupportInteractionBy to allow other interactor types
/// see UTATInteractionTargeterComponent::_CanInteractWithTarget()
class TAT_API ITATSupportInteractionByInterface
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintNativeEvent, Category = "Interact")
   bool DoesSupportInteractionBy(ACharacter* interactor, const FGameplayTag& interactorIdentity) const;
};


