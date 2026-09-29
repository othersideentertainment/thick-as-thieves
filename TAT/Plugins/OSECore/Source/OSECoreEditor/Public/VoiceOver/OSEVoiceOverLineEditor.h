// (c) 2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ose
#include "VoiceOver/OSEVoiceOverLine.h"

// ue4
#include "CoreMinimal.h"
#include "Toolkits/SimpleAssetEditor.h"

class FOSEVoiceOverLineEditor : public FSimpleAssetEditor
{
public:
   static TSharedRef<FOSEVoiceOverLineEditor> CreateEditor(const EToolkitMode::Type mode, const TSharedPtr<IToolkitHost> editWithinLevelEditor, UOSEVoiceOverLine* voiceLine);

   void InitVoiceLineEditor(const EToolkitMode::Type mode, const TSharedPtr< class IToolkitHost >& initToolkitHost, UOSEVoiceOverLine* voiceLine);
private:

   UOSEVoiceOverLine* _voiceLine;

   void _FillToolbar(FToolBarBuilder& toolbarBuilder);
   void _OnAutoPopulateClicked();
};
