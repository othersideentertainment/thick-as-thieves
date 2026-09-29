// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Quests/Modules/TATQuestGraphUtil.h"

// tat
#include "Quests/Modules/TATQuestGraphNode.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestGraphUtil)


namespace TATQuestGraphUtil
{
   template<typename T>
   FORCEINLINE FStringFormatArg NumberToFormatString(T value)
   {
      static_assert(std::is_integral_v<T> || std::is_floating_point_v<T>, "Expected a numeric type");
      if constexpr (std::is_floating_point_v<T>)
      {
         return FStringFormatArg(value);
      }
      else if constexpr (std::is_unsigned_v<T>)
      {
         return FStringFormatArg(static_cast<uint64>(value));
      }
      else if constexpr (std::is_signed_v<T>)
      {
         return FStringFormatArg(static_cast<int64>(value));
      }
      else
      {
         return FStringFormatArg(value);
      }
   }

   FString VariantToDebugString(const FVariant& value, bool includeTypeName)
   {
      auto formatVariantDebugString = [includeTypeName](const TCHAR* variantType, FStringFormatArg&& str) -> FString
      {
         return includeTypeName
            ? FString::Format(TEXT("Variant<{0}>({1})"), { variantType, MoveTemp(str) })
            : FString::Format(TEXT("{0}"), { MoveTemp(str) });
      };

#define TAT_VARIANT_TO_STRING(VARIANT_TYPE_NAME, TYPE) \
   EVariantTypes::VARIANT_TYPE_NAME: \
      return formatVariantDebugString(TEXT(#VARIANT_TYPE_NAME), value.GetValue<TYPE>().ToString())

#define TAT_VARIANT_TO_STRING_NUMERIC(VARIANT_TYPE_NAME, NUMERIC_TYPE) \
   EVariantTypes::VARIANT_TYPE_NAME: \
      static_assert(std::is_integral_v<NUMERIC_TYPE> || std::is_floating_point_v<NUMERIC_TYPE>, "Expected " #NUMERIC_TYPE " to be a numeric type"); \
      return formatVariantDebugString(TEXT(#VARIANT_TYPE_NAME), NumberToFormatString<NUMERIC_TYPE>(value.GetValue<NUMERIC_TYPE>()))

      switch (value.GetType())
      {
      case EVariantTypes::Empty:
         return formatVariantDebugString(TEXT("Empty"), TEXT(""));
      case EVariantTypes::Ansichar:
      {
         const ANSICHAR buf[] = { value.GetValue<ANSICHAR>(), 0 };
         return formatVariantDebugString(TEXT("Ansichar"), buf);
      }
      case EVariantTypes::Bool:
         return formatVariantDebugString(TEXT("Bool"), (value.GetValue<bool>() == true) ? TEXT("true") : TEXT("false"));
      case TAT_VARIANT_TO_STRING(Box, FBox);
      case TAT_VARIANT_TO_STRING(BoxSphereBounds, FBoxSphereBounds);
      // case EVariantTypes::ByteArray:
      //    return formatVariantDebugString(TEXT("ByteArray"), TEXT(""));
      case TAT_VARIANT_TO_STRING(Color, FColor);
      case TAT_VARIANT_TO_STRING(DateTime, FDateTime);
      case TAT_VARIANT_TO_STRING_NUMERIC(Double, double);
      case TAT_VARIANT_TO_STRING_NUMERIC(Enum, uint8);
      case TAT_VARIANT_TO_STRING_NUMERIC(Float, float);
      case TAT_VARIANT_TO_STRING(Guid, FGuid);
      case TAT_VARIANT_TO_STRING_NUMERIC(Int8, int8);
      case TAT_VARIANT_TO_STRING_NUMERIC(Int16, int16);
      case TAT_VARIANT_TO_STRING_NUMERIC(Int32, int32);
      case TAT_VARIANT_TO_STRING_NUMERIC(Int64, int64);
      //case TAT_VARIANT_TO_STRING(IntRect, FIntRect);
      case TAT_VARIANT_TO_STRING(LinearColor, FLinearColor);
      case TAT_VARIANT_TO_STRING(Matrix, FMatrix);
      case TAT_VARIANT_TO_STRING(Name, FName);
      case TAT_VARIANT_TO_STRING(Plane, FPlane);
      case TAT_VARIANT_TO_STRING(Quat, FQuat);
      //case TAT_VARIANT_TO_STRING(RandomStream, FRandomStream);
      case TAT_VARIANT_TO_STRING(Rotator, FRotator);
      case EVariantTypes::String:
         return formatVariantDebugString(TEXT("String"), value.GetValue<FString>());
      case EVariantTypes::Widechar:
      {
         const WIDECHAR buf[] = { value.GetValue<WIDECHAR>(), 0 };
         return formatVariantDebugString(TEXT("Widechar"), buf);
      }
      case TAT_VARIANT_TO_STRING(Timespan, FTimespan);
      case TAT_VARIANT_TO_STRING(Transform, FTransform);
      case TAT_VARIANT_TO_STRING(TwoVectors, FTwoVectors);
      case TAT_VARIANT_TO_STRING_NUMERIC(UInt8, uint8);
      case TAT_VARIANT_TO_STRING_NUMERIC(UInt16, uint16);
      case TAT_VARIANT_TO_STRING_NUMERIC(UInt32, uint32);
      case TAT_VARIANT_TO_STRING_NUMERIC(UInt64, uint64);
      case TAT_VARIANT_TO_STRING(Vector, FVector);
      case TAT_VARIANT_TO_STRING(Vector2d, FVector2d);
      case TAT_VARIANT_TO_STRING(Vector4, FVector4);
      case TAT_VARIANT_TO_STRING(IntPoint, FIntPoint);
      case TAT_VARIANT_TO_STRING(IntVector, FIntVector);
      case TAT_VARIANT_TO_STRING(NetworkGUID, FNetworkGUID);
      default:
         break;
      }

#undef TAT_VARIANT_TO_STRING
#undef TAT_VARIANT_TO_STRING_NUMERIC

      return TEXT("Variant(Unsupported)");
   }

   FString VariantToDebugString(const TOptional<FVariant>& value, bool includeTypeName)
   {
      return value.IsSet() ? VariantToDebugString(value.GetValue(), includeTypeName) : TEXT("Null");
   }

   FNodeBodyBuilder::FNodeBodyBuilder(const TSharedPtr<SVerticalBox>& body, const FNodeBodyStyle* defaultStyle)
      : BodyWidget(body)
      , DefaultStyle(defaultStyle)
   {
   }

   // static
   FNodeBodyBuilder FNodeBodyBuilder::Construct(const FNodeBodyStyle* defaultStyle)
   {
      return FNodeBodyBuilder(SNew(SVerticalBox), defaultStyle);
   }

   const FNodeBodyStyle& FNodeBodyBuilder::GetStyle(const FNodeBodyStyle* styleOverride) const
   {
      if (styleOverride != nullptr)
      {
         return *styleOverride;
      }
      if (DefaultStyle != nullptr)
      {
         return *DefaultStyle;
      }
      static const FNodeBodyStyle fallbackStyle = {};
      return fallbackStyle;
   }

   void FNodeBodyBuilder::AddContentSlot(const TSharedRef<SWidget>& content, TAttribute<EVisibility> visibilityAttr, float backgroundAlpha,
      EHorizontalAlignment horizAlign, const FNodeBodyStyle* styleOverride)
   {
      check(BodyWidget != nullptr);
      const FNodeBodyStyle& style = GetStyle(styleOverride);
      FLinearColor bgColor = style.TextBoxBackgroundColor;
      bgColor.A = backgroundAlpha;
      BodyWidget->AddSlot()
         .AutoHeight()
         .Padding(style.SlotPadding)
         .HAlign(horizAlign)
         [
            SNew(SBorder)
            .Padding(style.TextBoxPadding)
            .BorderImage(style.TextBoxBrush)
            .BorderBackgroundColor(FSlateColor(bgColor))
            .ForegroundColor(style.TextBoxTextColor)
            .Visibility(visibilityAttr)
            [
               content
            ]
         ];
   }
} // namespace TATQuestGraphUtil


// static
FString UTATQuestGraphFunctionLibrary::QuestGraphLogMessageToString(const FTATQuestGraphLogMessage& msg)
{
   const FString nodeDesc = (msg.SourceNode != nullptr) ? FString::Printf(TEXT("[%s] "), *msg.SourceNode->GetNodeDebugName()) : FString();
   const TCHAR* prefix = msg.IsError ? TEXT("ERROR: ") : TEXT("");
   return FString::Printf(TEXT("%s%s%s"), *nodeDesc, prefix, *msg.Message);
}

// static
bool UTATQuestGraphFunctionLibrary::GetProperty_Float(const FTATQuestGraphEvalContext& ctx, ETATQuestGraphPropertyType propertyType, float& result, float defaultValue)
{
   if (const TOptional<float> value = TATQuestGraphUtil::TryGetProperty<float>(ctx.Properties, propertyType))
   {
      result = value.GetValue();
      return true;
   }
   result = defaultValue;
   return false;
}
