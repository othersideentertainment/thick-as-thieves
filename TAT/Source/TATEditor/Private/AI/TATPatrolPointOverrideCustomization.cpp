// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "TATPatrolPointOverrideCustomization.h"

// tat 
#include "AI/Patrol/PatrolPath.h"

// ue
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "IDetailChildrenBuilder.h"
#include "AI/NavigationSystemBase.h"
#include "AI/Navigation/NavigationDataInterface.h"

void FTATPatrolPointOverrideCustomization::CustomizeHeader(
   TSharedRef<IPropertyHandle> inStructPropertyHandle,
   FDetailWidgetRow& headerRow,
   IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   headerRow
   .NameContent()
   [
      SNew(SHorizontalBox)
      +SHorizontalBox::Slot()
      [
         SNew(STextBlock)
         .Text(FText::FromString(FString::Printf(TEXT("Patrol Point %i"), inStructPropertyHandle.Get().GetArrayIndex())))
         .Font(IDetailLayoutBuilder::GetDetailFont())
      ]
   ]
   .ValueContent()
   [
      SNew(SHorizontalBox)
      +SHorizontalBox::Slot()
      .VAlign(VAlign_Center)
      [
         SNew(SButton)
         .OnClicked(this, &FTATPatrolPointOverrideCustomization::OnFocusSelected, inStructPropertyHandle)
         .Text(FText::FromString(TEXT("Focus")))
         ]
      +SHorizontalBox::Slot()
      .VAlign(VAlign_Center)
      [
         SNew(SButton)
         .OnClicked(this, &FTATPatrolPointOverrideCustomization::OnSnapSelected, inStructPropertyHandle)
         .Text(FText::FromString(TEXT("Snap")))
      ]
   ];
}

void FTATPatrolPointOverrideCustomization::CustomizeChildren(const TSharedRef<IPropertyHandle> inStructPropertyHandle,
   IDetailChildrenBuilder& structBuilder,
   IPropertyTypeCustomizationUtils& structCustomizationUtils)
{
   uint32 numChildren;
   inStructPropertyHandle->GetNumChildren(numChildren);
   for (uint32 childIndex = 0; childIndex < numChildren; ++childIndex)
   {
      const TSharedRef<IPropertyHandle> childHandle = inStructPropertyHandle->GetChildHandle(childIndex).ToSharedRef();
      structBuilder.AddProperty(childHandle);
   }
}

FReply FTATPatrolPointOverrideCustomization::OnSnapSelected(TSharedRef<IPropertyHandle> propertyHandle)
{
   TArray<UObject*> myObjects;
   propertyHandle->GetOuterObjects(myObjects);
   
   for (int32 objectIdx = 0; objectIdx < myObjects.Num(); objectIdx++)
   {
      if(const AActor* parentObject = Cast<AActor>(myObjects[objectIdx]))
      {
         const TSharedPtr<IPropertyHandle> patrolPointPositionHandle = propertyHandle.Get().GetChildHandle(GET_MEMBER_NAME_CHECKED(FPatrolPoint, Position));
         FVector positionValue;
         patrolPointPositionHandle.Get()->GetValue(positionValue);
         FNavLocation outLocation;

         UWorld* world = parentObject->GetWorld();
         if(world == nullptr)
            return FReply::Handled();

         const UNavigationSystemBase* navigationSystemBase = world->GetNavigationSystem();
         if(navigationSystemBase == nullptr)
            return FReply::Handled();
         const INavigationDataInterface* navData = navigationSystemBase->GetMainNavData();
         if(navData == nullptr)
            return FReply::Handled();
         if(navData->
         ProjectPoint(
            parentObject->ActorToWorld().TransformPosition(positionValue),
            outLocation,
            FVector(100, 100, 500)
            ))
         {
            patrolPointPositionHandle->SetValue(parentObject->ActorToWorld().InverseTransformPosition(outLocation.Location));
         }
         return FReply::Handled();
      }
   }
   return FReply::Handled();
}

FReply FTATPatrolPointOverrideCustomization::OnFocusSelected(TSharedRef<IPropertyHandle> propertyHandle)
{
   TArray<UObject*> myObjects;
   propertyHandle->GetOuterObjects(myObjects);
   
   for (int32 objectIdx = 0; objectIdx < myObjects.Num(); objectIdx++)
   {
      if(const AActor* parentObject = Cast<AActor>(myObjects[objectIdx]))
      {
         const TSharedPtr<IPropertyHandle> patrolPointPositionHandle = propertyHandle.Get().GetChildHandle(GET_MEMBER_NAME_CHECKED(FPatrolPoint, Position));
         FVector positionValue;
         patrolPointPositionHandle.Get()->GetValue(positionValue);

         const FVector positionToMoveTo = parentObject->GetActorLocation() + positionValue;
         GEditor->MoveViewportCamerasToBox(FBox(positionToMoveTo,positionToMoveTo).ExpandBy(300.f), true);
         return FReply::Handled();
      }
   }
   return FReply::Handled();
}
