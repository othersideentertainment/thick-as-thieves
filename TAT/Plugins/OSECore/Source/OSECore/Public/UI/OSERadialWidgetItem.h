// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ose
#include "OSEUserWidget.h"
#include "OSERadialWidget.h"

#include "OSERadialWidgetItem.generated.h"

UCLASS(meta = (DisableNativeTick))
class OSECORE_API UOSERadialWidgetItem : public UOSEUserWidget
{
   GENERATED_BODY()

public:
   // from UUserWidget
   virtual void NativeConstruct() override;
   virtual void NativeDestruct() override;

   void InitRadialItem(const FOSERadialItemInfo& item, int32 index, const FFloatInterval& itemArcAngleDegrees);
   void SetHovered(bool hovered);

   // Updates the radial item info in-place
   // NB. This explicitly does *NOT* update the item's arc angle and weight to avoid needing to recalculate the sizes of all other radial items.
   void UpdateRadialItem(const FOSERadialItemInfo& newItemInfo);

   UFUNCTION(BlueprintPure)
   int32 GetIndex() const { return _index; }
   UFUNCTION(BlueprintPure)
   FFloatInterval GetArcAngleDegrees() const { return _itemArcAngleDegrees; }
   UFUNCTION(BlueprintPure)
   const FOSERadialItemInfo& GetRadialItemInfo() const { return _itemInfo; }

protected:
   UFUNCTION(BlueprintNativeEvent, Category = "Radial Widget Item")
   void _OnInitRadialItem(const FOSERadialItemInfo& item, int32 index, const FFloatInterval& itemArcAngleDegrees);
   void _OnInitRadialItem_Implementation(const FOSERadialItemInfo& item, int32 index, const FFloatInterval& itemArcAngleDegrees) { }

   UFUNCTION(BlueprintNativeEvent, Category = "Radial Widget Item")
   void _OnRadialItemHoverChanged(bool hovered);
   void _OnRadialItemHoverChanged_Implementation(bool hovered) { }

private:
   FOSERadialItemInfo _itemInfo;
   int32 _index = INDEX_NONE;
   FFloatInterval _itemArcAngleDegrees{ 0.0f, 90.0f };
   bool _hovered = false;
};
