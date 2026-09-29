// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Widgets/SCompoundWidget.h"

class TAT_API STATWatermarkWidget : public SCompoundWidget
{
   SLATE_DECLARE_WIDGET(STATWatermarkWidget, SCompoundWidget)

public:

   SLATE_BEGIN_ARGS(STATWatermarkWidget)
      : _Content()
      , _Username(FText())
      , _UniqueId(FText())
      {}
      SLATE_DEFAULT_SLOT(FArguments, Content)

      SLATE_ARGUMENT(FText, Username)
      SLATE_ARGUMENT(FText, UniqueId)

   SLATE_END_ARGS()

   void Construct(const FArguments& args);
};

class TAT_API STATUserInfoWidget : public SCompoundWidget
{
   SLATE_DECLARE_WIDGET(STATUserInfoWidget, SCompoundWidget)

public:

   SLATE_BEGIN_ARGS(STATUserInfoWidget)
      : _Content()
      , _Username(FText())
      , _UniqueId(FText())
      {}
      SLATE_DEFAULT_SLOT(FArguments, Content)

      SLATE_ARGUMENT(FText, Username)
      SLATE_ARGUMENT(FText, UniqueId)

   SLATE_END_ARGS()

   STATUserInfoWidget() {}
   void Construct(const FArguments& args);
};
