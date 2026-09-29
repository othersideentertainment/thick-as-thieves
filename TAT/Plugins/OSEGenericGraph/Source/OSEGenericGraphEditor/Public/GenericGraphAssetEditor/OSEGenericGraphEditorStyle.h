// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"
#include "Styling/SlateStyle.h"

class OSEGENERICGRAPHEDITOR_API FOSEGenericGraphEditorStyle
{
public:
   static void Initialize();
   static void Shutdown();

   static const FName& GetStyleSetName();

private:
   static TSharedPtr<FSlateStyleSet> StyleSet;
};
