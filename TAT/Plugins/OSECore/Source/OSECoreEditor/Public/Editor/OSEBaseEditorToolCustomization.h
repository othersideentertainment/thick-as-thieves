// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"

// ue4
#include "IDetailCustomization.h"

class IDetailLayoutBuilder;

class OSECOREEDITOR_API FOSEBaseEditorToolCustomization : public IDetailCustomization
{
public:
   // Makes a new instance of this detail layout class for a specific detail view requesting it
   static TSharedRef<IDetailCustomization> MakeInstance();

   // from IDetailCustomization
   virtual void CustomizeDetails(IDetailLayoutBuilder& detailBuilder) override;

private:
   static FReply ExecuteToolCommand(IDetailLayoutBuilder* detailBuilder, UFunction* functionToExecute);

};
