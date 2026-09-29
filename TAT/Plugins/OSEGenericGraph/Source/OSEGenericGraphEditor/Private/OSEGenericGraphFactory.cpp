// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
// Based on https://github.com/jinyuliao/GenericGraph [(c) 2016 jinyuliao, MIT License]

#include "OSEGenericGraphFactory.h"

// 5/31/2024: This factory class allows creating graph types defined entirely in blueprints, which we don't want to support at this time
#if 0

#include "OSEGenericGraph.h"

#include "ClassViewerModule.h"
#include "ClassViewerFilter.h"
#include "Kismet2/KismetEditorUtilities.h"
#include "Kismet2/SClassPickerDialog.h"

#define LOCTEXT_NAMESPACE "OSEGenericGraphFactory"

class FAssetClassParentFilter : public IClassViewerFilter
{
public:
   FAssetClassParentFilter()
      : DisallowedClassFlags(CLASS_None), bDisallowBlueprintBase(false)
   {}

   /** All children of these classes will be included unless filtered out by another setting. */
   TSet< const UClass* > AllowedChildrenOfClasses;

   /** Disallowed class flags. */
   EClassFlags DisallowedClassFlags;

   /** Disallow blueprint base classes. */
   bool bDisallowBlueprintBase;

   virtual bool IsClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const UClass* InClass, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
   {
      bool bAllowed= !InClass->HasAnyClassFlags(DisallowedClassFlags)
         && InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InClass) != EFilterReturn::Failed;

      if (bAllowed && bDisallowBlueprintBase)
      {
         if (FKismetEditorUtilities::CanCreateBlueprintOfClass(InClass))
         {
            return false;
         }
      }

      return bAllowed;
   }

   virtual bool IsUnloadedClassAllowed(const FClassViewerInitializationOptions& InInitOptions, const TSharedRef< const IUnloadedBlueprintData > InUnloadedClassData, TSharedRef< FClassViewerFilterFuncs > InFilterFuncs) override
   {
      if (bDisallowBlueprintBase)
      {
         return false;
      }

      return !InUnloadedClassData->HasAnyClassFlags(DisallowedClassFlags)
         && InFilterFuncs->IfInChildOfClassesSet(AllowedChildrenOfClasses, InUnloadedClassData) != EFilterReturn::Failed;
   }
};


UOSEGenericGraphFactory::UOSEGenericGraphFactory()
{
   bCreateNew = true;
   bEditAfterNew = true;
   SupportedClass = UOSEGenericGraph::StaticClass();
}

UOSEGenericGraphFactory::~UOSEGenericGraphFactory()
{

}

bool UOSEGenericGraphFactory::ConfigureProperties()
{
   // nullptr the GenericGraphClass so we can check for selection
   GenericGraphClass = nullptr;

   // Load the classviewer module to display a class picker
   FClassViewerModule& ClassViewerModule = FModuleManager::LoadModuleChecked<FClassViewerModule>("ClassViewer");

   // Fill in options
   FClassViewerInitializationOptions Options;
   Options.Mode = EClassViewerMode::ClassPicker;

#if ENGINE_MAJOR_VERSION < 5
   TSharedPtr<FAssetClassParentFilter> Filter = MakeShareable(new FAssetClassParentFilter);
   Options.ClassFilter = Filter;
#else // #if ENGINE_MAJOR_VERSION < 5
   TSharedRef<FAssetClassParentFilter> Filter = MakeShareable(new FAssetClassParentFilter);
   Options.ClassFilters.Add(Filter);
#endif // #else // #if ENGINE_MAJOR_VERSION < 5

   Filter->DisallowedClassFlags = CLASS_Abstract | CLASS_Deprecated | CLASS_NewerVersionExists | CLASS_HideDropDown;
   Filter->AllowedChildrenOfClasses.Add(UOSEGenericGraph::StaticClass());

   const FText TitleText = LOCTEXT("CreateGenericGraphAssetOptions", "Pick Generic Graph Class");
   UClass* ChosenClass = nullptr;
   const bool bPressedOk = SClassPickerDialog::PickClass(TitleText, Options, ChosenClass, UOSEGenericGraph::StaticClass());

   if ( bPressedOk )
   {
      GenericGraphClass = ChosenClass;
   }

   return bPressedOk;
}

UObject* UOSEGenericGraphFactory::FactoryCreateNew(UClass* Class, UObject* InParent, FName Name, EObjectFlags Flags, UObject* Context, FFeedbackContext* Warn)
{
   if (GenericGraphClass != nullptr)
   {
      return NewObject<UOSEGenericGraph>(InParent, GenericGraphClass, Name, Flags | RF_Transactional);
   }
   else
   {
      check(Class->IsChildOf(UOSEGenericGraph::StaticClass()));
      return NewObject<UObject>(InParent, Class, Name, Flags | RF_Transactional);
   }

}

#undef LOCTEXT_NAMESPACE

#endif
