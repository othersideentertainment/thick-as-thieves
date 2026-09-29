// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Developer/TATDevToolTypes.h"

// imgui
#if TAT_ENABLE_DEV_TOOLS
#include "imgui.h"
#endif

// ue
#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

struct FTATDevToolState;

#if TAT_ENABLE_DEV_TOOLS

namespace TATImGui
{
   /// Simple wrapper around the buffer returned by StringCast that allows implicit conversion to const char*
   /// This is only really useful for passing strings directly to ImGui functions. For any other string conversion use-case, just use StringCast<ANSICHAR>() directly.
   struct FConvertedString
   {
      using CharType = ANSICHAR;
      using BufferType = decltype(StringCast<CharType>(TEXT("")));
      BufferType Buffer;
      explicit FConvertedString(const TCHAR* str) : Buffer(StringCast<CharType>(str)) {}
      FORCEINLINE operator const char*() const { return Buffer.Get(); }
   };

   FORCEINLINE FConvertedString ConvertString(const TCHAR* str) { return FConvertedString(str); }
   FORCEINLINE FConvertedString ConvertString(const FString& str) { return FConvertedString(*str); }

   template<typename T>
   FORCEINLINE FString EnumValueToString(T value) { return StaticEnum<T>()->GetNameStringByValue(static_cast<int64>(value)); }

   template <typename FmtType, typename... TArgs>
   FORCEINLINE void Text(const FmtType& fmt, const TArgs&... args)
   {
      const auto str = StringCast<ANSICHAR>(*FString::Printf<FmtType, TArgs...>(fmt, args...));
      ImGui::TextUnformatted(str.Get(), str.Get() + str.Length());
   }

   FORCEINLINE void TextUnformatted(const TCHAR* string)
   {
      const auto str = StringCast<ANSICHAR>(string);
      ImGui::TextUnformatted(str.Get(), str.Get() + str.Length());
   }

   FORCEINLINE void TextUnformatted(const FString& string)
   {
      const auto str = StringCast<ANSICHAR>(*string);
      ImGui::TextUnformatted(str.Get(), str.Get() + str.Length());
   }

   struct FScopedID
   {
      explicit FScopedID(int64 index) { ImGui::PushID(index); }
      explicit FScopedID(const TCHAR* str) { auto buf = StringCast<ANSICHAR>(str); ImGui::PushID(buf.Get(), buf.Get() + buf.Length()); }
      explicit FScopedID(const FString& str) : FScopedID(*str) {}
      explicit FScopedID(FName name) : FScopedID(*name.ToString()) {}

      template<typename T> requires (!std::is_same_v<T, TCHAR> && !std::is_same_v<T, ANSICHAR>)
      explicit FScopedID(const T* ptr) { ImGui::PushID(ptr); }

      ~FScopedID() { ImGui::PopID(); }

      FScopedID(const FScopedID&) = delete;
      FScopedID(FScopedID&&) = delete;
      FScopedID& operator=(const FScopedID&) = delete;
      FScopedID& operator=(FScopedID&&) = delete;
   };

   template<typename T>
   constexpr FORCEINLINE ImGuiDataType GetDataType()
   {
      if constexpr (std::is_same_v<T, bool>) { return ImGuiDataType_Bool; }
      else if constexpr (std::is_same_v<T, int8>) { return ImGuiDataType_S8; }
      else if constexpr (std::is_same_v<T, uint8>) { return ImGuiDataType_U8; }
      else if constexpr (std::is_same_v<T, int16>) { return ImGuiDataType_S16; }
      else if constexpr (std::is_same_v<T, uint16>) { return ImGuiDataType_U16; }
      else if constexpr (std::is_same_v<T, int32>) { return ImGuiDataType_S32; }
      else if constexpr (std::is_same_v<T, uint32>) { return ImGuiDataType_U32; }
      else if constexpr (std::is_same_v<T, int64>) { return ImGuiDataType_S64; }
      else if constexpr (std::is_same_v<T, uint64>) { return ImGuiDataType_U64; }
      else if constexpr (std::is_same_v<T, float>) { return ImGuiDataType_Float; }
      else if constexpr (std::is_same_v<T, double>) { return ImGuiDataType_Double; }
      else if constexpr (std::is_same_v<T, char*> || std::is_same_v<T, const char*>) { return ImGuiDataType_String; }
      else
      {
         ([]<bool valid = false>() { static_assert(valid, "Unsupported data type"); })();
         return ImGuiDataType_COUNT;
      }
   }

   // Wrapper around ImGui::DragScalarN
#define TAT_DRAG_SCALAR_FUNC(FUNC_NAME, TYPE, VALUE_TYPE, NUM_COMPONENTS) \
   FORCEINLINE bool FUNC_NAME(const TCHAR* label, TYPE& value, float v_speed = 1.0f, const VALUE_TYPE* p_min = nullptr, const VALUE_TYPE* p_max = nullptr, const TCHAR* format = nullptr, ImGuiSliderFlags flags = 0) \
   { \
      return ImGui::DragScalarN(ConvertString(label), GetDataType<VALUE_TYPE>(), reinterpret_cast<VALUE_TYPE*>(&value), (NUM_COMPONENTS), v_speed, p_min, p_max, (format != nullptr) ? ConvertString(format) : nullptr, flags); \
   }

   template<typename T> TAT_DRAG_SCALAR_FUNC(DragVector, UE::Math::TVector2<T>, T, 2)
   template<typename T> TAT_DRAG_SCALAR_FUNC(DragVector, UE::Math::TVector<T>, T, 3)
   template<typename T> TAT_DRAG_SCALAR_FUNC(DragVector, UE::Math::TVector4<T>, T, 4)
   template<typename T> TAT_DRAG_SCALAR_FUNC(DragVector, UE::Math::TQuat<T>, T, 4)
   template<typename T> TAT_DRAG_SCALAR_FUNC(DragVector, UE::Math::TRotator<T>, T, 3)
   TAT_DRAG_SCALAR_FUNC(DragVector, FLinearColor, float, 4)
   TAT_DRAG_SCALAR_FUNC(DragVector, FColor, uint8, 4)

#undef TAT_DRAG_SCALAR_FUNC

   // Wrapper around ImGui::SliderScalarN
#define TAT_SLIDER_SCALAR_FUNC(FUNC_NAME, TYPE, VALUE_TYPE, NUM_COMPONENTS) \
   FORCEINLINE bool FUNC_NAME(const TCHAR* label, TYPE& value, const VALUE_TYPE* p_min = nullptr, const VALUE_TYPE* p_max = nullptr, const TCHAR* format = nullptr, ImGuiSliderFlags flags = 0) \
   { \
      return ImGui::SliderScalarN(ConvertString(label), GetDataType<VALUE_TYPE>(), reinterpret_cast<VALUE_TYPE*>(&value), NUM_COMPONENTS, p_min, p_max, (format != nullptr) ? ConvertString(format) : nullptr, flags); \
   }

   template<typename T> TAT_SLIDER_SCALAR_FUNC(SliderVector, UE::Math::TVector2<T>, T, 2)
   template<typename T> TAT_SLIDER_SCALAR_FUNC(SliderVector, UE::Math::TVector<T>, T, 3)
   template<typename T> TAT_SLIDER_SCALAR_FUNC(SliderVector, UE::Math::TVector4<T>, T, 4)
   template<typename T> TAT_SLIDER_SCALAR_FUNC(SliderVector, UE::Math::TQuat<T>, T, 4)
   template<typename T> TAT_SLIDER_SCALAR_FUNC(SliderVector, UE::Math::TRotator<T>, T, 3)
   TAT_SLIDER_SCALAR_FUNC(SliderVector, FLinearColor, float, 4)
   TAT_SLIDER_SCALAR_FUNC(SliderVector, FColor, uint8, 4)

#undef TAT_SLIDER_SCALAR_FUNC

   // Wrapper around ImGui::InputScalarN
#define TAT_INPUT_SCALAR_FUNC(FUNC_NAME, TYPE, VALUE_TYPE, NUM_COMPONENTS) \
   FORCEINLINE bool FUNC_NAME(const TCHAR* label, TYPE& value, const VALUE_TYPE* p_step = nullptr, const VALUE_TYPE* p_step_fast = nullptr, const TCHAR* format = nullptr, ImGuiInputTextFlags flags = 0) \
   { \
      return ImGui::InputScalarN(ConvertString(label), GetDataType<VALUE_TYPE>(), reinterpret_cast<VALUE_TYPE*>(&value), (NUM_COMPONENTS), p_step, p_step_fast, (format != nullptr) ? ConvertString(format) : nullptr, flags); \
   }

   template<typename T> TAT_INPUT_SCALAR_FUNC(InputVector, UE::Math::TVector2<T>, T, 2)
   template<typename T> TAT_INPUT_SCALAR_FUNC(InputVector, UE::Math::TVector<T>, T, 3)
   template<typename T> TAT_INPUT_SCALAR_FUNC(InputVector, UE::Math::TVector4<T>, T, 4)
   template<typename T> TAT_INPUT_SCALAR_FUNC(InputVector, UE::Math::TQuat<T>, T, 4)
   template<typename T> TAT_INPUT_SCALAR_FUNC(InputVector, UE::Math::TRotator<T>, T, 3)
   TAT_INPUT_SCALAR_FUNC(InputVector, FLinearColor, float, 4)
   TAT_INPUT_SCALAR_FUNC(InputVector, FColor, uint8, 4)

#undef TAT_INPUT_SCALAR_FUNC

   bool InputTransform(const TCHAR* label, FTransform& value, ImGuiInputTextFlags flags = 0);

   bool InputString(const TCHAR* label, FString& value, ImGuiInputTextFlags flags = 0);
   bool InputString(const TCHAR* label, FString& value, ImGuiInputTextFlags flags, TFunctionRef<int(ImGuiInputTextCallbackData*)> callback);

   bool InputEnum(const TCHAR* label, int64& value, const UEnum* enumType, ImGuiComboFlags flags = 0);

   template<typename T>
   FORCEINLINE bool InputEnum(const TCHAR* label, T& value, ImGuiComboFlags flags = 0)
   {
      int64 valueAsInt = static_cast<int64>(value);
      const bool modified = InputEnum(label, valueAsInt, StaticEnum<T>(), flags);
      if (modified)
      {
         value = static_cast<T>(valueAsInt);
      }
      return modified;
   }

   void ProgressBar(float fraction, FVector2f size = FVector2f::ZeroVector, const TCHAR* overlay = nullptr);
   FORCEINLINE void ProgressBar(float fraction, FVector2f size, const FString& overlay) { ProgressBar(fraction, size, *overlay); }

   /// Wrapper around ImGui::Begin for handling dev tool windows.
   /// Call EndDevToolWindow only if BeginDevToolWindow returns true!
   bool BeginDevToolWindow(FTATDevToolState& state, ImGuiWindowFlags flags = 0);
   void EndDevToolWindow();

   bool BeginColumnGroup(const char* strId, TConstArrayView<const TCHAR*> columns, ImGuiTableFlags flags = 0, const FVector2f& outerSize = FVector2f::ZeroVector, float innerWidth = 0.0f);
   FORCEINLINE void EndColumnGroup() { ImGui::EndTable(); }
   FORCEINLINE bool NextColumnGroupColumn() { return ImGui::TableNextColumn(); }

   bool BeginColumnGroupColumnPane(const char* strId, FVector2f size = FVector2f::ZeroVector, ImGuiChildFlags childFlags = ImGuiChildFlags_FrameStyle, ImGuiWindowFlags windowFlags = 0);
   FORCEINLINE void EndColumnGroupColumnPane() { ImGui::EndChild(); }

   /// Check if an FProperty is supported by the InputProperty function
   bool IsSupportedProperty(const FProperty* prop);

   /// ImGui widget for arbitrary Unreal properties. Check if the property is supported with IsSupportedProperty first!
   bool InputProperty(const FProperty* prop, void* valuePtr, bool showLabel = true);

   /// Calls a callback for all UObject properties in an object. Intended to support the InputProperty ImGui widget.
   void ForEachUObjectProperty(UObject* obj, TFunctionRef<void(FName, const FProperty*, void*)> callback, bool recursion = false);

   bool BeginPropertyEditor(const char* editorId, ImGuiTableFlags flags = 0);
   void EndPropertyEditor();

   /// Starts a new property row. Returns true if the row is visible.
   bool NextProperty(const TCHAR* propName);
   FORCEINLINE bool NextProperty(const FString& propName) { return NextProperty(*propName); }
   FORCEINLINE bool NextProperty(FName propName) { return NextProperty(*propName.ToString()); }

   struct FObjectPropertyEditor
   {
      TConstArrayView<TWeakObjectPtr<UObject>> ObjectList;
      TWeakObjectPtr<UObject>* SelectedObject = nullptr;
      TFunction<bool(const FProperty*)> IsPropertyVisible;
      ImGuiTextFilter Filter;

      void Draw();
   };

}

#endif // TAT_ENABLE_DEV_TOOLS
