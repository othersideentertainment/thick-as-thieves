// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "TATEscapePointWidget.h"
#include "TATUserWidget.h"

// ue
#include "CoreMinimal.h"

#include "TATEscapeRoutesWidget.generated.h"

UCLASS(Blueprintable, BlueprintType, meta = (DisableNativeTick))
class TAT_API UTATEscapeRoutesWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;

protected:
   UFUNCTION()
   void OnEscapeRouteAdded(ATATEscapePoint* escapePoint);
   UFUNCTION()
   void OnEscapeRouteRemoved(ATATEscapePoint* escapePoint);

   UFUNCTION(BlueprintImplementableEvent)
   UTATEscapePointWidget* CreateEscapePointWidget(ATATEscapePoint* escapePoint);
   
   UPROPERTY(BlueprintReadOnly)
   TMap<ATATEscapePoint*, UTATEscapePointWidget*> escapePointToWidget {};
};
