// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


// tat
#include "UI/TATEscapeRoutesWidget.h"
#include "GameFramework/EscapeRoutes/TATEscapeRouteWorldSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEscapeRoutesWidget)

void UTATEscapeRoutesWidget::NativeConstruct()
{
   Super::NativeConstruct();
   if(const UWorld* world = GetWorld())
   {
      if(UTATEscapeRouteWorldSubsystem* escapeRouteSubsystem = world->GetSubsystem<UTATEscapeRouteWorldSubsystem>())
      {
         escapeRouteSubsystem->OnEscapeRouteAdded.AddDynamic(this, &ThisClass::OnEscapeRouteAdded);
         escapeRouteSubsystem->OnEscapeRouteRemoved.AddDynamic(this, &ThisClass::OnEscapeRouteRemoved);

         // Catch any escape routes registered before this widget's construction
         for (ATATEscapePoint* escapePoint : escapeRouteSubsystem->GetAllCurrentlyValidEscapePoints())
         {
            OnEscapeRouteAdded(escapePoint);
         }
      }
   }
}

void UTATEscapeRoutesWidget::NativeDestruct()
{
   if(const UWorld* world = GetWorld())
   {
      if(UTATEscapeRouteWorldSubsystem* escapeRouteSubsystem = world->GetSubsystem<UTATEscapeRouteWorldSubsystem>())
      {
         escapeRouteSubsystem->OnEscapeRouteAdded.RemoveAll(this);
         escapeRouteSubsystem->OnEscapeRouteRemoved.RemoveAll(this);
      }
   }
   Super::NativeDestruct();
}

void UTATEscapeRoutesWidget::OnEscapeRouteAdded(ATATEscapePoint* escapePoint)
{
   check(IsValid(escapePoint));
   check(escapePointToWidget.Contains(escapePoint) == false);
   
   UTATEscapePointWidget* createdWidget = CreateEscapePointWidget(escapePoint);
   check(IsValid(createdWidget));
   escapePointToWidget.Add(escapePoint, createdWidget);
}

void UTATEscapeRoutesWidget::OnEscapeRouteRemoved(ATATEscapePoint* escapePoint)
{
   UTATEscapePointWidget* escapePointWidget = escapePointToWidget.FindAndRemoveChecked(escapePoint);
   check(IsValid(escapePointWidget));
   escapePointWidget->HandleEscapeRouteRemoved();
}
