// (c) 2020-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "Interactables/TATUIInteractable.h"

#include "TATThievesDenInteractable.generated.h"

class ATATThievesDenPlayerController;
enum class ETATThievesDenScreen : uint8;

UENUM(BlueprintType)
enum class ETATThievesDenInteractableMode : uint8
{
   AddWidgetToViewport,
   ShowThievesDenScreen,
   HighlightOnly,
   MAX UMETA(Hidden)
};

UCLASS(Blueprintable, BlueprintType)
class TAT_API ATATThievesDenInteractable : public ATATUIInteractable
{
   GENERATED_BODY()

public:
   ATATThievesDenInteractable();

protected:
   // from AActor
   virtual void BeginPlay() override;
   virtual void EndPlay(const EEndPlayReason::Type endPlayReason) override;

   // from ATATUIInteractable
   virtual void _OnShowUI(ATATPlayerController* localController) override;
   virtual void _OnHideUI() override;

   UFUNCTION(BlueprintNativeEvent, Category = "Thieves Den Interactable")
   void _OnThievesDenScreenAboutToShow(UTATScreenWidget* widget);
   virtual void _OnThievesDenScreenAboutToShow_Implementation(UTATScreenWidget* widget) {}

   UFUNCTION(BlueprintNativeEvent, Category = "Thieves Den Interactable")
   void _OnThievesDenScreenClosed(UTATScreenWidget* widget);
   virtual void _OnThievesDenScreenClosed_Implementation(UTATScreenWidget* widget) {}

   virtual FInteractStartResult StartInteract_Implementation(ACharacter* interactingCharacter) override;

public:

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thieves Den Interactable")
   ETATThievesDenInteractableMode InteractMode = ETATThievesDenInteractableMode::AddWidgetToViewport;

   UPROPERTY(BlueprintReadWrite, EditAnywhere, Category = "Thieves Den Interactable", meta = (EditCondition = "InteractMode == ETATThievesDenInteractableMode::ShowThievesDenScreen", EditConditionHides))
   ETATThievesDenScreen ThievesDenScreen = static_cast<ETATThievesDenScreen>(0);

private:
   UFUNCTION()
   void _OnThievesDenWidgetAdded(ETATThievesDenScreen screenType, UTATScreenWidget* widget);

   UFUNCTION()
   void _OnThievesDenWidgetRemoved(ETATThievesDenScreen screenType, UTATScreenWidget* widget);

   FDelegateHandle _thievesDenScreenChangeDelegate;
};
