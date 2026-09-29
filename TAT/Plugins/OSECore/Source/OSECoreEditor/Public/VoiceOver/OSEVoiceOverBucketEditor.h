// (c) 2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// ose
#include "VoiceOver/OSEVoiceOverBucket.h"

// ue4
#include "CoreMinimal.h"
#include "Toolkits/SimpleAssetEditor.h"

class FOSEVoiceOverBucketEditor : public FSimpleAssetEditor
{
public:
   static TSharedRef<FOSEVoiceOverBucketEditor> CreateEditor(const EToolkitMode::Type mode, const TSharedPtr<IToolkitHost> editWithinLevelEditor, UOSEVoiceOverBucket* voiceBucket);

   void InitVoiceLineEditor(const EToolkitMode::Type mode, const TSharedPtr< class IToolkitHost >& initToolkitHost, UOSEVoiceOverBucket* voiceBucket);
private:

   UOSEVoiceOverBucket* _voiceBucket;

   void _FillToolbar(FToolBarBuilder& toolbarBuilder);
};
