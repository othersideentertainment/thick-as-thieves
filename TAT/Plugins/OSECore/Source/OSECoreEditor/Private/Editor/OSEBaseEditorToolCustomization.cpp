// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Editor/OSEBaseEditorToolCustomization.h"

// ose
#include "Editor/OSEBaseEditorTool.h"

// ue4
#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "ObjectEditorUtils.h"

TSharedRef<IDetailCustomization> FOSEBaseEditorToolCustomization::MakeInstance()
{
   return MakeShareable(new FOSEBaseEditorToolCustomization);
}

void FOSEBaseEditorToolCustomization::CustomizeDetails(IDetailLayoutBuilder& detailBuilder)
{
   TSet<UClass*> toolClasses;

   TArray<TWeakObjectPtr<UObject>> objectsBeingCustomized;
   detailBuilder.GetObjectsBeingCustomized(objectsBeingCustomized);

   for (TWeakObjectPtr<UObject>& weakObjectBeingCustomized : objectsBeingCustomized)
   {
      if (UOSEBaseEditorTool* objectBeingCustomized = Cast<UOSEBaseEditorTool>(weakObjectBeingCustomized.Get()))
      {
         toolClasses.Add(objectBeingCustomized->GetClass());
      }
   }

   // create a button for each exec function
   for(UClass* toolClass : toolClasses)
   {
      for(TFieldIterator<UFunction> funcIt(toolClass); funcIt; ++funcIt)
      {
         UFunction* function = (*funcIt);

         // checking for 0 params because we don't have UI support (yet?) for inserting params
         if (function->HasAnyFunctionFlags(FUNC_Exec) && function->NumParms == 0)
         {
            const FText& functionName = function->GetDisplayNameText();
            const FString& categoryStr = function->HasMetaData(TEXT("Category")) ? function->GetMetaData(TEXT("Category")) : TEXT("Commands");
            
            IDetailCategoryBuilder& category = detailBuilder.EditCategory(*categoryStr);
            category.AddCustomRow(functionName)
               .ValueContent()
               [
                  SNew(SButton)
                  .Text(functionName)
                  .OnClicked(FOnClicked::CreateStatic(&FOSEBaseEditorToolCustomization::ExecuteToolCommand, &detailBuilder, function))
               ];
         }
      }
   }

   // hide anything with the "hidden" category
   detailBuilder.HideCategory("Hidden");
}

FReply FOSEBaseEditorToolCustomization::ExecuteToolCommand(IDetailLayoutBuilder* detailBuilder, UFunction* functionToExecute)
{
   TArray<TWeakObjectPtr<UObject>> objectsBeingCustomized;
   detailBuilder->GetObjectsBeingCustomized(objectsBeingCustomized);

   for (TWeakObjectPtr<UObject>& weakObjectBeingCustomized : objectsBeingCustomized)
   {
      if (UObject* objInstance = weakObjectBeingCustomized.Get())
      {
         objInstance->CallFunctionByNameWithArguments(*functionToExecute->GetName(), *GLog, nullptr, true);
      }
   }

   return FReply::Handled();
}
