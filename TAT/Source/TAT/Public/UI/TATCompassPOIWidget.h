// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "Blueprint/UserWidget.h"

// tat
#include "UI/TATUserWidget.h"

#include "TATCompassPOIWidget.generated.h"

// ue4
class UPaperSprite;
class FString;

// tat
class ATATGenericIndicator;

// Used to control the type of widget that represents the POI on the compass
UENUM(BlueprintType)
enum class EPOIDisplayMode : uint8
{
   Image,
   Text
};

UENUM(BlueprintType)
enum class EPOICategory : uint8
{
   Cardinal = 0,
   Generic,
   SecondaryObjective,
   PrimaryObjective
};

UCLASS()
class TAT_API UTATCompassPOIWidget : public UTATUserWidget
{
   GENERATED_BODY()
   
public:
   UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
   void SetPOISprite(const UPaperSprite* sprite);

   UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
   void SetPOIText(const FText& text);

   UFUNCTION(BlueprintCallable)
   void SetPOIOffScreenIndicator(bool isLeftOfScreen);

   UFUNCTION(BlueprintCallable)
   void ClearPOIOffScreenIndicator();

   UFUNCTION(BlueprintCallable)
   void SetPOIVerticalityIndicator(bool isAbovePlayer);

   UFUNCTION(BlueprintCallable)
   void ClearPOIVerticalityIndicator();

   UFUNCTION()
   void SetPOIVisible(bool visible);

   UFUNCTION(BlueprintCallable)
   void SetEnabled(bool enabled) { _enabled = enabled; }

   bool IsEnabled() const { return _enabled; }

   UFUNCTION(BlueprintImplementableEvent, BlueprintCallable)
   void AnimatePOISize(bool grow);

   UFUNCTION(BlueprintCallable)
   void UpdateDistance(float distance);

   UFUNCTION(BlueprintCallable)
   void SetDistanceVisible(bool visible);

protected:
   UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
   TObjectPtr<UWidget> Image_AboveIndicator;
   UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
   TObjectPtr<UWidget> Image_BelowIndicator;
   UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
   TObjectPtr<UWidget> Image_LeftIndicator;
   UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
   TObjectPtr<UWidget> Image_RightIndicator;
   
   UPROPERTY(BlueprintReadOnly, meta = (BindWidget))
   TObjectPtr<class UTextBlock> Text_Distance;

   UPROPERTY(EditDefaultsOnly)
   FText DistanceFormat;
   
private:
   // Controls whether a POI should be allowed to render on the compass
   bool _enabled = true;

   int _cachedDistance = -1;
};
