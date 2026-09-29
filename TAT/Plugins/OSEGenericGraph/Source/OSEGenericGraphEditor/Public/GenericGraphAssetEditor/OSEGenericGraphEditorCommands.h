// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"

class OSEGENERICGRAPHEDITOR_API FOSEGenericGraphEditorCommands : public TCommands<FOSEGenericGraphEditorCommands>
{
public:
   /** Constructor */
   FOSEGenericGraphEditorCommands()
      : TCommands<FOSEGenericGraphEditorCommands>("OSEGenericGraphEditor", NSLOCTEXT("Contexts", "OSEGenericGraphEditor", "OSE Generic Graph Editor"), NAME_None, FAppStyle::GetAppStyleSetName())
   {
   }

   TSharedPtr<FUICommandInfo> GraphSettings;
   TSharedPtr<FUICommandInfo> Refresh;
   TSharedPtr<FUICommandInfo> AutoArrange;

   virtual void RegisterCommands() override;
};
