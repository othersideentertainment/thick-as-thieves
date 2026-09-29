// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT
#pragma once

// tat
#include "Quests/Modules/TATQuestGraphTypes.h"

#include "TATQuestGraphUtil.generated.h"

namespace TATQuestGraphUtil
{
   template<typename T>
   TOptional<T> TryGetProperty(const TMap<ETATQuestGraphPropertyType, FVariant>& props, ETATQuestGraphPropertyType propType)
   {
      const FVariant* value = props.Find(propType);
      if (value == nullptr)
      {
         return NullOpt;
      }

      using Traits = TVariantTraits<T>;
      static constexpr EVariantTypes expectedType = Traits::GetType();

      const EVariantTypes actualType = value->GetType();

      // Allow getting doubles as floats and floats as doubles
      if constexpr (std::is_same_v<T, float>)
      {
         if (actualType == EVariantTypes::Double)
         {
            return static_cast<float>(value->GetValue<double>());
         }
      }
      else if constexpr (std::is_same_v<T, double>)
      {
         if (actualType == EVariantTypes::Float)
         {
            return static_cast<double>(value->GetValue<float>());
         }
      }

      if (actualType != expectedType)
      {
         return NullOpt;
      }

      return value->GetValue<T>();
   }

   FString VariantToDebugString(const FVariant& value, bool includeTypeName = true);
   FString VariantToDebugString(const TOptional<FVariant>& value, bool includeTypeName = true);

   template<typename RetType, typename... TArgs>
   using TStaticFn = RetType(*)(TArgs...);

   /// Constructs a TAttribute using a UObject owner pointer and a static function that takes exactly one parameter - the owning UObject pointer - and returns the attributes value.
   /// The callback will only be called while the UObject is valid, otherwise returns defaultValue.
   /// Intended to simplify attributes so they don't need to deal with lifetimes (or even check for null).
   ///
   /// NB. When passing a lambda argument without explicit template arguments, you need to use operator+ to coerce the type correctly.
   ///     For example: MakeAttribute(this, +[](auto* self) { return FText::GetEmpty(); })
   template<typename AttrT, typename ObjT>
   TAttribute<AttrT> MakeAttribute(ObjT* owner, TStaticFn<AttrT, ObjT*> callback, const AttrT& defaultValue = AttrT{})
   {
      if (owner == nullptr || callback == nullptr)
      {
         return defaultValue;
      }
      TWeakObjectPtr<ObjT> weakPtr = MakeWeakObjectPtr(owner);
      return TAttribute<AttrT>::CreateLambda([weakPtr, callback, defaultValue]() -> AttrT
      {
         ObjT* ptr = weakPtr.Get();
         return (ptr != nullptr) ? callback(ptr) : defaultValue;
      });
   }

   /// Similar to MakeAttribute, but takes a callback that returns a bool and returns TAttribute<EVisibility>.
   /// Returns EVisibility::Visible if the callback returns true, otherwise returns EVisibility::Collapsed.
   ///
   /// NB. When passing a lambda argument without explicit template arguments, you need to use operator+ to coerce the type correctly.
   ///     For example: MakeAttributeVisibilityFromBool(this, +[](auto* self) { return true; })
   template<typename ObjT>
   TAttribute<EVisibility> MakeAttributeVisibilityFromBool(ObjT* owner, TStaticFn<bool, ObjT*> callback, EVisibility defaultValue = EVisibility::Collapsed)
   {
      if (owner == nullptr || callback == nullptr)
      {
         return defaultValue;
      }
      TWeakObjectPtr<ObjT> weakPtr = MakeWeakObjectPtr(owner);
      return TAttribute<EVisibility>::CreateLambda([weakPtr, callback, defaultValue]() -> EVisibility
      {
         if (ObjT* ptr = weakPtr.Get())
         {
            return callback(ptr) ? EVisibility::Visible : EVisibility::Collapsed;
         }
         return defaultValue;
      });
   }

   struct FNodeBodyStyle
   {
      FMargin SlotPadding = FMargin(2.0f);
      FMargin TextBoxPadding = FMargin(6.0f);
      const FSlateBrush* TextBoxBrush = FAppStyle::Get().GetBrush("ColorPicker.RoundedSolidBackground");
      FLinearColor TextBoxBackgroundColor = FLinearColor(0.0f, 0.0f, 0.0f, 1.0f);
      FSlateColor TextBoxTextColor = FSlateColor(FColor::White);
   };

   struct FNodeBodyBuilder
   {
      TSharedPtr<SVerticalBox> BodyWidget;
      const FNodeBodyStyle* DefaultStyle = nullptr;

      explicit FNodeBodyBuilder(const TSharedPtr<SVerticalBox>& body, const FNodeBodyStyle* defaultStyle = nullptr);

      static FNodeBodyBuilder Construct(const FNodeBodyStyle* defaultStyle = nullptr);

      const FNodeBodyStyle& GetStyle(const FNodeBodyStyle* styleOverride = nullptr) const;

      void AddContentSlot(
         const TSharedRef<SWidget>& content,
         TAttribute<EVisibility> visibilityAttr = EVisibility::Visible,
         float backgroundAlpha = 1.0f,
         EHorizontalAlignment horizAlign = HAlign_Fill,
         const FNodeBodyStyle* styleOverride = nullptr);
   };
}


UCLASS()
class UTATQuestGraphFunctionLibrary : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:

   UFUNCTION(BlueprintPure, Category = "Quests", meta = (DisplayName = "To String", CompactNodeTitle = "->", Keywords = "cast convert", BlueprintAutocast))
   static FString QuestGraphLogMessageToString(const FTATQuestGraphLogMessage& msg);

   //UFUNCTION(BlueprintPure, Category = "Quests", DisplayName = "Get Quest Graph Property (Float)")
   static bool GetProperty_Float(const FTATQuestGraphEvalContext& ctx, ETATQuestGraphPropertyType propertyType, float& result, float defaultValue = 0.0f);
};
