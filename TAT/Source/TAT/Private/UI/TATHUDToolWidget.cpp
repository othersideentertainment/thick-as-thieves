// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "UI/TATHUDToolWidget.h"

#include "Tools/TATToolComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATHUDToolWidget)

DEFINE_LOG_CATEGORY_STATIC(LogTATHUDToolWidget, Log, All);

void UTATHUDToolWidget::PostConstructSetupToolWidget(UTATToolComponent* toolComponent)
{
   ensure(_ownerTool == nullptr);
   _ownerTool = toolComponent;
   OnToolWidgetCreated(toolComponent);
}
