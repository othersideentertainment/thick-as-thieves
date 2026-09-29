// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/TATSceneVariantCustomization.h"

// tat
#include "Variation/SceneVariants/TATSceneRequirement.h"

// ue5
#include "Widgets/DeclarativeSyntaxSupport.h"
#include "IDetailChildrenBuilder.h"
#include "PropertyHandle.h"
#include "PropertyCustomizationHelpers.h"

void FTATSceneSpawnerOverrideCustomization::CustomizeHeader(TSharedRef<IPropertyHandle> inStructPropertyHandle, FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   // Gets Type
   TSharedPtr<IPropertyHandle> key = inStructPropertyHandle->GetChildHandle(0);

   // Gets PercentC
   TSharedPtr<IPropertyHandle> value = inStructPropertyHandle->GetChildHandle(1);

   // Setup in the header row so that we still get the TArray dropdown
   headerRow
      .NameContent()
      [
         inStructPropertyHandle->CreatePropertyNameWidget()
      ]
      .ValueContent()
      .MaxDesiredWidth(0.0f)
      [
         SNew(SHorizontalBox)
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
               key->CreatePropertyValueWidget()
            ]
            + SHorizontalBox::Slot()
            .AutoWidth()
            [
               // TODO: visibility
               value->CreatePropertyValueWidget()
            ]
      ];
   // This avoids making duplicate reset boxes
   inStructPropertyHandle->MarkResetToDefaultCustomized();
}

void FTATSceneSpawnerOverrideCustomization::CustomizeChildren(TSharedRef<IPropertyHandle> inStructPropertyHandle, IDetailChildrenBuilder& structBuilder, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
}

void FTATSceneRequirementCustomization::CustomizeHeader(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class FDetailWidgetRow& headerRow, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   // equivalent-ish to acting as if not implemented
   constexpr TCHAR kShowOnlyInnerPropertiesMetaDataSpecifier[] = TEXT("ShowOnlyInnerProperties");
   const bool showHeader = !inStructPropertyHandle->HasMetaData(kShowOnlyInnerPropertiesMetaDataSpecifier);
   if (showHeader)
   {
      headerRow
         .NameContent()
         [
            inStructPropertyHandle->CreatePropertyNameWidget()
         ]
         .ValueContent()
         [
            inStructPropertyHandle->CreatePropertyValueWidget()
         ];
   }
}

void FTATSceneRequirementCustomization::CustomizeChildren(TSharedRef<class IPropertyHandle> inStructPropertyHandle, class IDetailChildrenBuilder& structBuilder, IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   TSharedRef<class IPropertyHandle> typeHandle = inStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTATSceneRequirement, Type)).ToSharedRef();
   structBuilder.AddProperty(typeHandle);
   structBuilder.AddProperty(inStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTATSceneRequirement, Scene)).ToSharedRef());
   structBuilder.AddProperty(inStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTATSceneRequirement, Variant)).ToSharedRef());
   TSharedRef<class IPropertyHandle> elementTagHandle = inStructPropertyHandle->GetChildHandle(GET_MEMBER_NAME_CHECKED(FTATSceneRequirement, ElementTag)).ToSharedRef();
   IDetailPropertyRow& elementTagRow = structBuilder.AddProperty(elementTagHandle);

   auto hasTemplate = [&inStructPropertyHandle]() -> bool
      {
         TArray<UObject*> outerObjects;
         inStructPropertyHandle->GetOuterObjects(outerObjects);
         return outerObjects.ContainsByPredicate([](UObject* obj) { return obj && obj->IsTemplate(); });
      };

   // Want always show the element tag in templates, so it can be edited there
   if (!hasTemplate())
   {
      // If not template, hide if not the right type (the other properties do this with edit conditions)
      elementTagRow.Visibility(TAttribute<EVisibility>::CreateLambda([typeHandle]() {
         uint8 value = 0;
         bool shouldShow = typeHandle->GetValue(value) == FPropertyAccess::MultipleValues || static_cast<ETATSceneRequirementType>(value) == ETATSceneRequirementType::RequireSceneElement;
         return shouldShow ? EVisibility::Visible : EVisibility::Collapsed;
      }));
   }
}
