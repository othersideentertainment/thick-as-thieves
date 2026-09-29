// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#pragma once

#include "CoreMinimal.h"

class FOSEGenericGraphAssetEditor;
class FExtender;
class FToolBarBuilder;

class OSEGENERICGRAPHEDITOR_API FOSEGenericGraphAssetEditorToolbar : public TSharedFromThis<FOSEGenericGraphAssetEditorToolbar>
{
public:
   FOSEGenericGraphAssetEditorToolbar(TSharedPtr<FOSEGenericGraphAssetEditor> InGenericGraphEditor)
      : GenericGraphEditor(InGenericGraphEditor) {}

   void AddGenericGraphToolbar(TSharedPtr<FExtender> Extender);

private:
   void FillGenericGraphToolbar(FToolBarBuilder& ToolbarBuilder);

protected:
   /** Pointer back to the blueprint editor tool that owns us */
   TWeakPtr<FOSEGenericGraphAssetEditor> GenericGraphEditor;
};
