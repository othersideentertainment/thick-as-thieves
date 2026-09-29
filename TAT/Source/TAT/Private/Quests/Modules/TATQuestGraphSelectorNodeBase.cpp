// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphSelectorNodeBase.h"

// tat
#include "Quests/Modules/TATQuestGraphUtil.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphSelectorNodeBase)


FString UTATQuestGraphSelectorNodeBase::GetNodeDebugName() const
{
#if WITH_EDITOR
   const int32 numChoices = _GetSelectorNumChoices();
   TArray<FString, TInlineAllocator<6>> parts = {
      ((SelectCount.Min > 1 || SelectCount.Max > 1) ? _GetSelectorTypeDisplayNamePlural() : _GetSelectorTypeDisplayName()).ToString(),
      (SelectCount.Min == SelectCount.Max)
         ? FString::Printf(TEXT("SelectCount=%i"), SelectCount.Min)
         : FString::Printf(TEXT("SelectCount=[%i..%i]"), SelectCount.Min, SelectCount.Max),
   };

   if (numChoices == 0)
   {
      parts.Add(TEXT("EMPTY"));
   }
   else if (numChoices == 1)
   {
      parts.Add(_GetSelectorValueColumnText(0).ToString());
   }
   else
   {
      parts.Add(FString::Printf(TEXT("%i Choices"), numChoices));
      const int32 numChoicesToShow = FMath::Min(numChoices, 3);
      for (int32 i = 0; i < numChoicesToShow; i++)
      {
         parts.Add(FString::Printf(TEXT("[%s] %s"), *_GetSelectorKeyColumnText(i).ToString(), *_GetSelectorValueColumnText(i).ToString()));
      }
      if (numChoicesToShow < numChoices)
      {
         parts.Add(TEXT("..."));
      }
   }
   return FString::Printf(TEXT("SelectorNode(%s)"), *FString::Join(parts, TEXT(", ")));
#else
   return TEXT("SelectorNode");
#endif
}

#if WITH_EDITOR

void UTATQuestGraphSelectorNodeBase::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   if (_listWidget == nullptr)
   {
      return;
   }
   const FName selectorListPropName = _GetSelectorListPropertyName();
   if (selectorListPropName != NAME_None && selectorListPropName == propertyChangedEvent.GetPropertyName())
   {
      _RefreshSelectorList();
   }
}

TSharedPtr<SWidget> UTATQuestGraphSelectorNodeBase::ConstructNodeBodyWidget()
{
   if (_listWidget == nullptr)
   {
      _listWidget = SNew(SVerticalBox);
   }
   TATQuestGraphUtil::FNodeBodyBuilder builder = TATQuestGraphUtil::FNodeBodyBuilder::Construct();
   _ConstructNodeBodyContentSlots(builder);
   return builder.BodyWidget;
}

FText UTATQuestGraphSelectorNodeBase::GetNodeDisplayTitle() const
{
   static const FText selectText = INVTEXT("Select");
   static const FText addText = INVTEXT("Add");

   TArray<FText> parts = {
      (_GetSelectorNumChoices() == 1) ? addText : selectText,
   };

   const FText selectTypeDisplayName = (SelectCount.Min > 1 || SelectCount.Max > 1) ? _GetSelectorTypeDisplayNamePlural() : _GetSelectorTypeDisplayName();
   if (!selectTypeDisplayName.IsEmpty())
   {
      parts.Add(selectTypeDisplayName);
   }

   if (SelectCount != FInt32Interval(1, 1))
   {
      if (SelectCount.Min == SelectCount.Max)
      {
         parts.Add(FText::FormatOrdered(INVTEXT("[{0}]"), SelectCount.Min));
      }
      else if (SelectCount.Max == SelectCount.Min + 1)
      {
         parts.Add(FText::FormatOrdered(INVTEXT("[{0} or {1}]"), SelectCount.Min, SelectCount.Max));
      }
      else
      {
         parts.Add(FText::FormatOrdered(INVTEXT("[{0} to {1}]"), SelectCount.Min, SelectCount.Max));
      }
   }

   return FText::Join(INVTEXT(" "), parts);
}

FText UTATQuestGraphSelectorNodeBase::GetNodeDisplaySubtitle() const
{
   if (_GetSelectorNumChoices() == 1)
   {
      return _GetSelectorValueColumnText(0);
   }
   return FText::GetEmpty();
}

void UTATQuestGraphSelectorNodeBase::_ConstructNodeBodyContentSlots(TATQuestGraphUtil::FNodeBodyBuilder& builder)
{
   _RefreshSelectorList();

   static constexpr float backgroundAlpha = 0.75f;
   builder.AddContentSlot(
      _listWidget.ToSharedRef(),
      TATQuestGraphUtil::MakeAttributeVisibilityFromBool(this, +[](UTATQuestGraphSelectorNodeBase* self) { return self->_GetSelectorNumChoices() >= 2; }),
      backgroundAlpha);
}

TSharedPtr<SWidget> UTATQuestGraphSelectorNodeBase::_ConstructSelectorItemWidget(int32 idx) const
{
   return SNew(SHorizontalBox)
      +SHorizontalBox::Slot()
      .AutoWidth()
      .Padding(FMargin(2.0f))
      [
         SNew(SBox)
         .MinDesiredWidth(40.0f)
         [
            SNew(STextBlock)
            .Font(FAppStyle::GetFontStyle("BoldFont"))
            .Text_Lambda([idx, weakThis = MakeWeakObjectPtr(this)]() -> FText
            {
               const UTATQuestGraphSelectorNodeBase* self = weakThis.Get();
               return (self != nullptr) ? self->_GetSelectorKeyColumnText(idx) : FText::GetEmpty();
            })
         ]
      ]
      +SHorizontalBox::Slot()
      .AutoWidth()
      .Padding(FMargin(2.0f))
      [
         SNew(STextBlock)
            .Text_Lambda([idx, weakThis = MakeWeakObjectPtr(this)]() -> FText
            {
               const UTATQuestGraphSelectorNodeBase* self = weakThis.Get();
               return (self != nullptr) ? self->_GetSelectorValueColumnText(idx) : FText::GetEmpty();
            })
      ];
}

void UTATQuestGraphSelectorNodeBase::_RefreshSelectorList()
{
   check(_listWidget != nullptr);

   const int32 numChoices = _GetSelectorNumChoices();

   if (_listWidget->NumSlots() == numChoices)
   {
      return;
   }

   if (numChoices == 0)
   {
      _listWidget->ClearChildren();
      return;
   }

   if (_listWidget->NumSlots() < numChoices)
   {
      int32 slotIndex = _listWidget->NumSlots();
      while (_listWidget->NumSlots() < numChoices)
      {
         if (const TSharedPtr<SWidget> itemWidget = _ConstructSelectorItemWidget(slotIndex))
         {
            _listWidget->AddSlot().AttachWidget(itemWidget.ToSharedRef());
         }
         ++slotIndex;
      }
   }
   else
   {
      check(_listWidget->NumSlots() > numChoices);
      while (_listWidget->NumSlots() > numChoices)
      {
         TSharedRef<SWidget> lastWidget = _listWidget->GetSlot(_listWidget->NumSlots() - 1).GetWidget();
         _listWidget->RemoveSlot(lastWidget);
      }
   }
}

#endif // WITH_EDITOR
