// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "UI/TATUserWidget.h"

// ue5
#include "CoreMinimal.h"
#include "UObject/Object.h"


#include "TATWorldSpaceUserWidget.generated.h"

UENUM(BlueprintType)
enum class EWorldWidgetClampingType : uint8
{
   None,
   ClampToEdgeOfScreen,
   ClampToCenterOfScreen
};

UENUM(BlueprintType)
enum class EWorldWidgetRotationType : uint8
{
   None,
   SpecifyRotationBasedOnEdgeDirection,
   RotateToFaceAwayFromCenterWhenAtEdge,
   AlwaysRotateToFaceAwayFromCenter
};

// Base class for displaying widgets in worldspace, with optional clamping.
UCLASS(meta = (DisableNativeTick))
class TAT_API UTATWorldSpaceUserWidget : public UTATUserWidget
{
   GENERATED_BODY()

public:
   virtual void NativeOnInitialized() override;

   UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
   void TickScreenPosition();

   UFUNCTION(BlueprintPure, BlueprintNativeEvent)
   FVector2D GetWidgetSize() const;

   UFUNCTION(BlueprintNativeEvent)
   void OnWidgetClampStateChanged(bool bIsClamped);

   UFUNCTION(BlueprintPure)
   bool IsWidgetClamped() const { return _bIsWidgetClamped; }

   UFUNCTION(BlueprintPure, BlueprintNativeEvent)
   UWidget* GetWidgetToRotate();
   
   virtual void SetIsEnabled(bool bInIsEnabled) override;
   
   UFUNCTION(BlueprintCallable)
   void SetIsEnabledAndShown(bool isEnabledAndShown);
   
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnShouldBeVisibleChanged, bool, shouldBeVisible);
   UPROPERTY(BlueprintAssignable)
   FOnShouldBeVisibleChanged OnShouldBeVisible;
   
protected:
   UFUNCTION(BlueprintPure)
   FVector2D GetScreenPositionFromWorldLocation(APlayerController* playerController, FVector worldLocation, bool& bDidProjectBackwards, bool bAllowReverseProjection =
                                                   false) const;
   
   bool _ClampLocation(FVector2D& inOutLocation, const FVector2D& widgetSize, const FVector2D& screenSize) const;
   bool _ClampToEdgeOfScreen(FVector2D& inOutLocation, const FVector2D& widgetSize, const FVector2D& screenSize) const;
   bool _ClampToCenterOfScreen(FVector2D& inOutLocation, const FVector2D& widgetSize, const FVector2D& screenSize) const;

   void _RotateWidget(const FVector2D& inLocation, const FVector2D& screenSize, const FVector2D& widgetSize);
   void _RotateWidgetTowardsCenterOfScreen(const FVector2D& inLocation, const FVector2D& screenSize);
   void _RotateWidgetToSpecifiedAmountBasedOnEdgeDirection(const FVector2D& inLocation, const FVector2D& screenSize, const FVector2D& widgetSize);
   
   UPROPERTY(BlueprintReadOnly, Transient, Meta=(ExposeOnSpawn=true))
   TObjectPtr<USceneComponent> _sceneComponentToTrack = {nullptr};
   
   UPROPERTY(Transient)
   TObjectPtr<APlayerController> _localPlayerController = {nullptr};
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|UI")
   EWorldWidgetClampingType _clampingType = {EWorldWidgetClampingType::None};

   UPROPERTY(EditDefaultsOnly, Category="TAT|UI")
   EWorldWidgetRotationType _rotationType = {EWorldWidgetRotationType::None};

   UPROPERTY(EditDefaultsOnly, Category="TAT|UI", Meta = (EditCondition = "_rotationType == EWorldWidgetRotationType::SpecifyRotationBasedOnEdgeDirection", EditConditionHides, ClampMin = "0.0", UIMin = "0.0", ClampMax = "360.0", UIMax = "360.0"))
   float degreesAtNorthEdgeOfScreen = 0.0f;
   UPROPERTY(EditDefaultsOnly, Category="TAT|UI", Meta = (EditCondition = "_rotationType == EWorldWidgetRotationType::SpecifyRotationBasedOnEdgeDirection", EditConditionHides, ClampMin = "0.0", UIMin = "0.0", ClampMax = "360.0", UIMax = "360.0"))
   float degreesAtSouthEdgeOfScreen = 0.0f;
   UPROPERTY(EditDefaultsOnly, Category="TAT|UI", Meta = (EditCondition = "_rotationType == EWorldWidgetRotationType::SpecifyRotationBasedOnEdgeDirection", EditConditionHides, ClampMin = "0.0", UIMin = "0.0", ClampMax = "360.0", UIMax = "360.0"))
   float degreesAtEastEdgeOfScreen = 0.0f;
   UPROPERTY(EditDefaultsOnly, Category="TAT|UI", Meta = (EditCondition = "_rotationType == EWorldWidgetRotationType::SpecifyRotationBasedOnEdgeDirection", EditConditionHides, ClampMin = "0.0", UIMin = "0.0", ClampMax = "360.0", UIMax = "360.0"))
   float degreesAtWestEdgeOfScreen = 0.0f;

   UPROPERTY(EditDefaultsOnly, Category="TAT|UI", meta=(ClampMin=0.0f, ClampMax=1.0f))
   float _maxDistanceFromCenterOfScreen = {.5f};
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|UI")
   bool _allowReverseProjectionOfWorldPosition = {false};
   
   
   UPROPERTY(EditDefaultsOnly, Category="TAT|UI")
   bool _clampToLowerHalfOfScreen { false };
   bool _bIsWidgetClamped = {false};

   UPROPERTY(BlueprintReadOnly)
   bool _bIsProjectedBackwards = {false};
};
