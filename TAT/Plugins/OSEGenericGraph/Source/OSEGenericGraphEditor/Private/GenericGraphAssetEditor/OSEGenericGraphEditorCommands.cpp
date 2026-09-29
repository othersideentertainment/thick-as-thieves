// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "GenericGraphAssetEditor/OSEGenericGraphEditorCommands.h"

#define LOCTEXT_NAMESPACE "EditorCommands_GenericGraph"

void FOSEGenericGraphEditorCommands::RegisterCommands()
{
   UI_COMMAND(GraphSettings, "Graph Settings", "Graph Settings", EUserInterfaceActionType::Button, FInputChord());
   UI_COMMAND(Refresh, "Refresh", "Graph Settings", EUserInterfaceActionType::Button, FInputChord());
   UI_COMMAND(AutoArrange, "Auto Arrange", "Auto Arrange", EUserInterfaceActionType::Button, FInputChord());
}

#undef LOCTEXT_NAMESPACE
