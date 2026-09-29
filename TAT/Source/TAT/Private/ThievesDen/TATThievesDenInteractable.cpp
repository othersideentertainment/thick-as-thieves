// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "ThievesDen/TATThievesDenInteractable.h"

// tat
#include "ThievesDen/TATThievesDenPlayerController.h"
#include "ThievesDen/TATThievesDenGameState.h"
#include "TATGameInstance.h"

// ue
#include "Blueprint/UserWidget.h"
#include "GameFramework/Character.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThievesDenInteractable)

ATATThievesDenInteractable::ATATThievesDenInteractable()
{
}

void ATATThievesDenInteractable::BeginPlay()
{
   Super::BeginPlay();

   for (ULocalPlayer* localPlayer : UTATGameInstance::Get(this).GetLocalPlayers())
   {
      if (ATATThievesDenPlayerController* pc = Cast<ATATThievesDenPlayerController>(localPlayer->PlayerController))
      {
         pc->OnThievesDenScreenWidgetAdded.AddUniqueDynamic(this, &ATATThievesDenInteractable::_OnThievesDenWidgetAdded);
         pc->OnThievesDenScreenWidgetRemoved.AddUniqueDynamic(this, &ATATThievesDenInteractable::_OnThievesDenWidgetRemoved);
      }
   }
}

void ATATThievesDenInteractable::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   for (ULocalPlayer* localPlayer : UTATGameInstance::Get(this).GetLocalPlayers())
   {
      if (ATATThievesDenPlayerController* pc = Cast<ATATThievesDenPlayerController>(localPlayer->PlayerController))
      {
         pc->OnThievesDenScreenWidgetAdded.RemoveDynamic(this, &ATATThievesDenInteractable::_OnThievesDenWidgetAdded);
         pc->OnThievesDenScreenWidgetRemoved.RemoveDynamic(this, &ATATThievesDenInteractable::_OnThievesDenWidgetRemoved);
      }
   }

   Super::EndPlay(endPlayReason);
}

void ATATThievesDenInteractable::_OnShowUI(ATATPlayerController* localController)
{
   switch (InteractMode)
   {
   case ETATThievesDenInteractableMode::AddWidgetToViewport:
      Super::_OnShowUI(localController);
      break;
   case ETATThievesDenInteractableMode::ShowThievesDenScreen:
      if (ATATThievesDenPlayerController* pc = Cast<ATATThievesDenPlayerController>(localController))
      {
         pc->ServerRequestSetThievesDenScreen(ThievesDenScreen);
      }
      else
      {
         _OnHideUI();
      }
      break;
   case ETATThievesDenInteractableMode::HighlightOnly:
      // do nothing
      break;
   default:
      checkNoEntry();
      break;
   }
}

void ATATThievesDenInteractable::_OnHideUI()
{
   Super::_OnHideUI();
}

FInteractStartResult ATATThievesDenInteractable::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   if (InteractMode == ETATThievesDenInteractableMode::HighlightOnly)
   {
      return FInteractStartResult();
   }

   return Super::StartInteract_Implementation(interactingCharacter);
}

void ATATThievesDenInteractable::_OnThievesDenWidgetAdded(ETATThievesDenScreen screenType, UTATScreenWidget* widget)
{
   if (InteractMode != ETATThievesDenInteractableMode::ShowThievesDenScreen || screenType != ThievesDenScreen)
   {
      return;
   }
   _OnThievesDenScreenAboutToShow(widget);
}

void ATATThievesDenInteractable::_OnThievesDenWidgetRemoved(ETATThievesDenScreen screenType, UTATScreenWidget* widget)
{
   if (InteractMode != ETATThievesDenInteractableMode::ShowThievesDenScreen || screenType != ThievesDenScreen)
   {
      return;
   }
   _OnThievesDenScreenClosed(widget);
   _OnHideUI();
}
