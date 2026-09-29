// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Containers/Set.h"
#include "Templates/SubclassOf.h"

#include "TATThiefToolEntryMgr.generated.h"

class UTATThiefToolEntryWidget;
class UToolComponent;
class UWidget;

// Container used for storage/lookup of tool entry widgets
USTRUCT()
struct TAT_API FTATThiefToolEntryMgr
{
   GENERATED_BODY()

public:
   // Creates tool entry widget of given type, adding to collection
   UTATThiefToolEntryWidget* ConstructToolEntryWidget(const UToolComponent* toolComponent, TSubclassOf<UTATThiefToolEntryWidget> toolEntryWidgetClass, UWidget* ownerWidget);
   UTATThiefToolEntryWidget* GetWidgetForTool(const UToolComponent* toolComponent) const;
   const TSet<UTATThiefToolEntryWidget*>& GetAllToolWidgets() const { return _toolEntryWidgets; }

   void RemoveToolEntryWidget(UTATThiefToolEntryWidget* toolEntryWidget);
   void ClearToolEntryWidgets();

private:
   UPROPERTY(Transient)
   TSet<UTATThiefToolEntryWidget*> _toolEntryWidgets;
};
