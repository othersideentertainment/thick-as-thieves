// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/OSERadialWidgetItem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSERadialWidgetItem)

// ose

// ue4

void UOSERadialWidgetItem::NativeConstruct()
{
   Super::NativeConstruct();
}

void UOSERadialWidgetItem::NativeDestruct()
{
   Super::NativeDestruct();
}

void UOSERadialWidgetItem::InitRadialItem(const FOSERadialItemInfo& item, int32 index, const FFloatInterval& itemArcAngleDegrees)
{
   _itemInfo = item;
   _index = index;
   _itemArcAngleDegrees = itemArcAngleDegrees;

   _OnInitRadialItem(_itemInfo, _index, _itemArcAngleDegrees);

   SetHovered(false);
}

void UOSERadialWidgetItem::SetHovered(bool hovered)
{
   if (_hovered != hovered)
   {
      _hovered = hovered;
      _OnRadialItemHoverChanged(_hovered);
   }
}

void UOSERadialWidgetItem::UpdateRadialItem(const FOSERadialItemInfo& newItemInfo)
{
   constexpr bool allowUpdateWeight = false;
   _itemInfo.UpdateInPlace(newItemInfo, allowUpdateWeight);
}
