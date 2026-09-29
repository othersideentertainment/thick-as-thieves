// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// tat
#include "UI/TATUserWidget.h"
#include "WorldMap/TATWorldMapTypes.h"


#include "TATWorldMapActorWidget.generated.h"

UCLASS(Blueprintable, meta = (DisableNativeTick))
class TAT_API UTATWorldMapActorWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   UFUNCTION(BlueprintImplementableEvent)
   void SetMapActorSprite(const FTATMapSpriteEntry& mapSpriteEntry);

   UFUNCTION(BlueprintImplementableEvent)
   void SetMapLabel(const FText& mapLabel);

   UFUNCTION(BlueprintImplementableEvent)
   void SetMapActorFacingAngle(FVector2D facingAngle);

   UFUNCTION(BlueprintImplementableEvent)
   void RefreshMapActorVisuals(const FTATMapRepresentationData& mapRepresentationData, bool shouldShowOnMap);
};
