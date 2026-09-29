// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Developer/TATImGuiHelpers.h"

// tat
#include "Developer/TATDevToolSubsystem.h"

#if TAT_ENABLE_DEV_TOOLS

namespace TATImGui
{
   struct FInputStringUserData
   {
      static constexpr int32 BufferSize = 96;
      TArray<char, TInlineAllocator<BufferSize>>* Buffer;
      ImGuiInputTextCallback ChainCallback;
      void* ChainCallbackUserData;
   };

   static int InputStringInternalCallback(ImGuiInputTextCallbackData* data)
   {
      check(data != nullptr);
      FInputStringUserData* userdata = static_cast<FInputStringUserData*>(data->UserData);
      check(userdata != nullptr);
      if (data->EventFlag == ImGuiInputTextFlags_CallbackResize)
      {
         // Resize string callback
         // If for some reason we refuse the new length (BufTextLen) and/or capacity (BufSize) we need to set them back to what we want.
         check(userdata->Buffer != nullptr);
         IM_ASSERT(data->Buf == userdata->Buffer->GetData());
         userdata->Buffer->SetNum(data->BufTextLen);
         data->Buf = userdata->Buffer->GetData();
      }
      else if (userdata->ChainCallback)
      {
         // Forward to user callback, if any
         data->UserData = userdata->ChainCallbackUserData;
         return userdata->ChainCallback(data);
      }
      return 0;
   }

   static bool InputStringInternal(const TCHAR* label, FString& value, ImGuiInputTextFlags flags, ImGuiInputTextCallback callback, void* callbackData)
   {
      constexpr int32 bufferSize = FInputStringUserData::BufferSize;

      // Make a string buffer we can pass to ImGui for editing, convert the input string to a char*, then copy the converted string to the buffer
      TArray<char, TInlineAllocator<bufferSize>> buffer;
      auto conv = StringCast<ANSICHAR>(*value);
      buffer.SetNumZeroed(FMath::Max(conv.Length(), bufferSize) + 8);
      check(conv.Length() <= buffer.Num());
      for (int32 i = 0; i < conv.Length(); i++)
      {
         FMemory::Memcpy(buffer.GetData(), conv.Get(), conv.Length());
      }

      FInputStringUserData userdata = {
         &buffer,
         callback,
         callbackData,
      };

      const bool modified = ImGui::InputText(ConvertString(label), buffer.GetData(), buffer.Num(), flags, callback, &userdata);

      if (modified)
      {
         // If any text was modified, convert the buffer back to TCHAR and assign the value back to the input string
         value = StringCast<TCHAR>(buffer.GetData());
      }

      return modified;
   }

   bool InputTransform(const TCHAR* label, FTransform& value, ImGuiInputTextFlags flags)
   {
      bool modified = false;

      FScopedID id{ label };

      if (label != nullptr && label[0] != '#')
      {
         ImGui::AlignTextToFramePadding();
         TATImGui::TextUnformatted(label);
         ImGui::SameLine();
      }

      ImGui::BeginGroup();

      // Not sure why this is needed, but the first input vector call doesn't align right without it when we have no label within a property editor
      ImGui::Dummy({ 0, 0 });

      FVector loc = value.GetLocation();
      if (TATImGui::InputVector<FVector::FReal>(TEXT("Location"), loc, nullptr, nullptr, TEXT("%.3f"), flags))
      {
         value.SetLocation(loc);
         modified = true;
      }

      FRotator rot = value.GetRotation().Rotator();
      if (TATImGui::InputVector<FRotator::FReal>(TEXT("Rotation"), rot, nullptr, nullptr, TEXT("%.2f d"), flags))
      {
         value.SetRotation(rot.Quaternion());
         modified = true;
      }

      FVector scale = value.GetScale3D();
      if (TATImGui::InputVector<FVector::FReal>(TEXT("Scale"), scale, nullptr, nullptr, TEXT("%.3f"), flags))
      {
         value.SetScale3D(scale);
         modified = true;
      }

      ImGui::EndGroup();

      return modified;
   }

   bool InputString(const TCHAR* label, FString& value, ImGuiInputTextFlags flags)
   {
      return InputStringInternal(label, value, flags, nullptr, nullptr);
   }

   struct FInputStringCallbackAdapter
   {
      TFunctionRef<int(ImGuiInputTextCallbackData*)> UserCallback;
   };

   static int InputStringCallbackWrapper(ImGuiInputTextCallbackData* data)
   {
      check(data != nullptr);
      check(data->UserData != nullptr);
      FInputStringCallbackAdapter* adapter = static_cast<FInputStringCallbackAdapter*>(data->UserData);
      return adapter->UserCallback(data);
   }

   bool InputString(const TCHAR* label, FString& value, ImGuiInputTextFlags flags, TFunctionRef<int(ImGuiInputTextCallbackData*)> callback)
   {
      FInputStringCallbackAdapter adapter{ callback };
      return InputStringInternal(label, value, flags, InputStringCallbackWrapper, &adapter);
   }

   bool InputEnum(const TCHAR* label, int64& value, const UEnum* enumType, ImGuiComboFlags flags)
   {
      bool modified = false;
      if (ensure(enumType))
      {
         const FString curValueLabel = enumType->GetNameStringByValue(value);
         if (ImGui::BeginCombo(ConvertString(label), ConvertString(curValueLabel), flags))
         {
            for (int32 i = 0; i < enumType->NumEnums(); i++)
            {
               const int64 thisValue = enumType->GetValueByIndex(i);
               const bool selected = thisValue == value;
               FScopedID valueId{ thisValue };
               if (ImGui::Selectable(ConvertString(enumType->GetNameStringByIndex(i)), selected))
               {
                  value = thisValue;
                  modified = true;
               }
            }
            ImGui::EndCombo();
         }
      }
      return modified;
   }

   void ProgressBar(float fraction, FVector2f size, const TCHAR* overlay)
   {
      if (overlay == nullptr)
      {
         ImGui::ProgressBar(fraction, size);
      }
      else
      {
         ImGui::ProgressBar(fraction, size, ConvertString(overlay));
      }
   }

   bool BeginDevToolWindow(FTATDevToolState& state, ImGuiWindowFlags flags)
   {
      if (!state.IsOpen)
      {
         return false;
      }
      ImGui::Begin(ConvertString(state.Label), &state.IsOpen, flags);
      return true;
   }

   void EndDevToolWindow()
   {
      ImGui::End();
   }

   bool BeginColumnGroup(const char* strId, TConstArrayView<const TCHAR*> columns, ImGuiTableFlags flags, const FVector2f& outerSize, float innerWidth)
   {
      flags |= ImGuiTableFlags_Resizable;
      const bool isOpen = ImGui::BeginTable(strId, columns.Num(), flags, outerSize, innerWidth);
      if (isOpen)
      {
         for (const TCHAR* columnLabel : columns)
         {
            ImGui::TableSetupColumn(ConvertString(columnLabel));
         }
      }
      return isOpen;
   }

   bool BeginColumnGroupColumnPane(const char* strId, FVector2f size, ImGuiChildFlags childFlags, ImGuiWindowFlags windowFlags)
   {
      if (!NextColumnGroupColumn())
      {
         return false;
      }
      ImGui::BeginChild(strId, size, childFlags, windowFlags);
      return true;
   }

   bool IsSupportedProperty(const FProperty* prop)
   {
      if (prop == nullptr)
      {
         return false;
      }

      // No array support yet
      if (prop->ArrayDim != 1)
      {
         return false;
      }

      // Base types are easy to support
      if (prop->IsA<FBoolProperty>()
         || (prop->IsA<FByteProperty>() && CastFieldChecked<FByteProperty>(prop)->IsEnum())
         || prop->IsA<FIntProperty>()
         || prop->IsA<FFloatProperty>()
         || prop->IsA<FDoubleProperty>()
         || prop->IsA<FStrProperty>()
         )
      {
         return true;
      }

      if (prop->IsA<FStructProperty>())
      {
         const FStructProperty* structProp = CastField<FStructProperty>(prop);
         if (structProp == nullptr)
         {
            return false;
         }

         if (structProp->Struct == TBaseStructure<FColor>::Get()
            || structProp->Struct == TBaseStructure<FLinearColor>::Get()
            || structProp->Struct == TBaseStructure<FVector2D>::Get()
            || structProp->Struct == TBaseStructure<FVector>::Get()
            || structProp->Struct == TBaseStructure<FRotator>::Get()
            || structProp->Struct == TBaseStructure<FQuat>::Get())
         {
            return true;
         }

#if 0
         //TODO: GameplayTag support
         if (structProp->Struct == FGameplayTag::StaticStruct())
         {
            return true;
         }
#endif
      }

      return false;
   }

   template<typename ValueType, typename PropType>
   struct FPropWrapper
   {
      static constexpr bool IsStruct = std::is_same_v<PropType, FStructProperty>;

      const PropType* Property = nullptr;
      void* ValuePtr = nullptr;

      FPropWrapper(const FProperty* prop, void* valuePtr)
         : Property(CastField<PropType>(prop))
         , ValuePtr(valuePtr)
      {
         if constexpr (IsStruct)
         {
            // invalidate this object if the struct type doesn't match
            if (Property && (Property->Struct != TBaseStructure<ValueType>::Get() && Property->GetOwnerStruct() != TBaseStructure<ValueType>::Get()))
            {
               Property = nullptr;
            }
         }
      }

      FORCEINLINE explicit operator bool() const
      {
         return Property != nullptr && ValuePtr != nullptr;
      }

      ValueType GetValue() const
      {
         if constexpr (IsStruct)
         {
            return *static_cast<ValueType*>(ValuePtr);
         }
         else
         {
            check(Property != nullptr && ValuePtr != nullptr);
            return Property->GetPropertyValue(ValuePtr);
         }
      }

      void SetValue(const ValueType& newValue)
      {
         if constexpr (IsStruct)
         {
            *static_cast<ValueType*>(ValuePtr) = newValue;
         }
         else
         {
            check(Property != nullptr && ValuePtr != nullptr);
            Property->SetPropertyValue(ValuePtr, newValue);
         }
      }
   };

   template<typename ValueType, typename PropType>
   void SetPropValueInternal(const FProperty* prop, void* valuePtr, const ValueType& newValue)
   {
      const PropType* typedProp = CastField<PropType>(prop);
      check(typedProp != nullptr);
      typedProp->SetPropertyValue(valuePtr, newValue);
   }

   bool InputProperty(const FProperty* prop, void* valuePtr, bool showLabel)
   {
      FScopedID propId{ prop };

      if (auto wrap = FPropWrapper<bool, FBoolProperty>(prop, valuePtr))
      {
         bool value = wrap.GetValue();
         const bool modified = ImGui::Checkbox((showLabel ? ConvertString(prop->GetName()) : ""), &value);
         if (modified)
         {
            wrap.SetValue(value);
         }
         return modified;
      }

      if (auto wrap = FPropWrapper<uint8, FByteProperty>(prop, valuePtr))
      {
         if (wrap.Property->IsEnum())
         {
            check(wrap.Property->Enum != nullptr);

            int64 value = wrap.GetValue();
            const bool modified = InputEnum((showLabel ? *prop->GetName() : TEXT("")), value, wrap.Property->Enum);
            if (modified)
            {
               wrap.SetValue(static_cast<uint8>(value));
            }
            return modified;
         }
      }

      if (auto wrap = FPropWrapper<int32, FIntProperty>(prop, valuePtr))
      {
         int32 value = wrap.GetValue();
         const bool modified = ImGui::InputInt((showLabel ? ConvertString(prop->GetName()) : ""), &value);
         if (modified)
         {
            wrap.SetValue(value);
         }
         return modified;
      }

      if (auto wrap = FPropWrapper<float, FFloatProperty>(prop, valuePtr))
      {
         float value = wrap.GetValue();
         const bool modified = ImGui::InputFloat((showLabel ? ConvertString(prop->GetName()) : ""), &value);
         if (modified)
         {
            wrap.SetValue(value);
         }
         return modified;
      }

      if (auto wrap = FPropWrapper<double, FDoubleProperty>(prop, valuePtr))
      {
         double value = wrap.GetValue();
         const bool modified = ImGui::InputDouble((showLabel ? ConvertString(prop->GetName()) : ""), &value);
         if (modified)
         {
            wrap.SetValue(value);
         }
         return modified;
      }

      if (auto wrap = FPropWrapper<FString, FStrProperty>(prop, valuePtr))
      {
         FString value = wrap.GetValue();
         const bool modified = InputString((showLabel ? *prop->GetName() : TEXT("")), value);
         if (modified)
         {
            wrap.SetValue(value);
         }
         return modified;
      }

      if (auto wrap = FPropWrapper<FColor, FStructProperty>(prop, valuePtr))
      {
         FLinearColor value = FLinearColor(wrap.GetValue());
         const bool modified = ImGui::ColorEdit4((showLabel ? ConvertString(prop->GetName()) : ""), reinterpret_cast<float*>(&value));
         if (modified)
         {
            constexpr bool srgb = false;
            wrap.SetValue(value.ToFColor(srgb));
         }
         return modified;
      }

      if (auto wrap = FPropWrapper<FLinearColor, FStructProperty>(prop, valuePtr))
      {
         FLinearColor value = wrap.GetValue();
         const bool modified = ImGui::ColorEdit4((showLabel ? ConvertString(prop->GetName()) : ""), reinterpret_cast<float*>(&value));
         if (modified)
         {
            wrap.SetValue(value);
         }
         return modified;
      }

#define TAT_TRY_VECTOR_TYPE(TYPE) \
      if (auto wrap = FPropWrapper<TYPE, FStructProperty>(prop, valuePtr)) \
      { \
         TYPE value = wrap.GetValue(); \
         const bool modified = DragVector((showLabel ? *prop->GetName() : TEXT("")), value); \
         if (modified) \
         { \
            wrap.SetValue(value); \
         } \
         return modified; \
      }

      TAT_TRY_VECTOR_TYPE(FVector2D)
      TAT_TRY_VECTOR_TYPE(FVector)
      TAT_TRY_VECTOR_TYPE(FRotator)
      TAT_TRY_VECTOR_TYPE(FQuat)

#undef TAT_TRY_VECTOR_TYPE

      ensure(IsSupportedProperty(prop));
      return false;
   }

   void ForEachUObjectProperty(UObject* obj, TFunctionRef<void(FName, const FProperty*, void*)> callback, bool recursion)
   {
      if (obj == nullptr)
      {
         return;
      }

      UClass* selfClass = obj->GetClass();
      check(selfClass != nullptr);

      const EPropertyValueIteratorFlags flags = recursion ? EPropertyValueIteratorFlags::FullRecursion : EPropertyValueIteratorFlags::NoRecursion;

      for (TPropertyValueIterator<FProperty> propIter(selfClass, obj, flags); propIter; ++propIter)
      {
         const FProperty* prop = propIter->Key;
         check(prop != nullptr);
         callback(prop->GetFName(), prop, prop->ContainerPtrToValuePtr<uint8>(obj));
      }
   }

   bool BeginPropertyEditor(const char* editorId, ImGuiTableFlags flags)
   {
      flags |= ImGuiTableFlags_Resizable;
      flags |= ImGuiTableFlags_ScrollY;
      const bool isOpen = ImGui::BeginTable(editorId, 2, flags);
      if (isOpen)
      {
         ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
         ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 2.0f); // Default twice larger
      }
      return isOpen;
   }

   void EndPropertyEditor()
   {
      ImGui::EndTable();
   }

   bool NextProperty(const TCHAR* propName)
   {
      ImGui::TableNextRow();
      if (ImGui::TableNextColumn())
      {
         ImGui::AlignTextToFramePadding();
         TATImGui::TextUnformatted(propName);
      }
      if (ImGui::TableNextColumn())
      {
         ImGui::SetNextItemWidth(-FLT_MIN);
         return true;
      }
      return false;
   }

   void FObjectPropertyEditor::Draw()
   {
      // Left side: draw object list
      // - Currently using a table to benefit from RowBg feature
      if (ImGui::BeginChild("##ObjectList", ImVec2(300, 0), ImGuiChildFlags_ResizeX | ImGuiChildFlags_Borders | ImGuiChildFlags_NavFlattened))
      {
         ImGui::SetNextItemWidth(-FLT_MIN);
         ImGui::SetNextItemShortcut(ImGuiMod_Ctrl | ImGuiKey_F, ImGuiInputFlags_Tooltip);
         ImGui::PushItemFlag(ImGuiItemFlags_NoNavDefaultFocus, true);
         if (ImGui::InputTextWithHint("##Filter", "incl,-excl", Filter.InputBuf, IM_ARRAYSIZE(Filter.InputBuf), ImGuiInputTextFlags_EscapeClearsAll))
         {
            Filter.Build();
         }
         ImGui::PopItemFlag();

         if (ImGui::BeginTable("##Background", 1, ImGuiTableFlags_RowBg))
         {
            for (const TWeakObjectPtr<UObject>& weakObj : ObjectList)
            {
               UObject* obj = weakObj.Get();
               if (obj == nullptr)
               {
                  continue;
               }

               if (Filter.PassFilter(ConvertString(obj->GetName()))) // Filter root node
               {
                  ImGui::TableNextRow();
                  ImGui::TableNextColumn();
                  TATImGui::FScopedID thisObj{ obj };
                  ImGuiTreeNodeFlags treeFlags = ImGuiTreeNodeFlags_None;
                  treeFlags |= ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick; // Standard opening mode as we are likely to want to add selection afterwards
                  treeFlags |= ImGuiTreeNodeFlags_NavLeftJumpsBackHere; // Left arrow support

                  if (SelectedObject != nullptr && SelectedObject->Get() == obj)
                     treeFlags |= ImGuiTreeNodeFlags_Selected;

                  // if (node->Childs.Size == 0)
                  //    treeFlags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_Bullet;

                  // if (node->DataMyBool == false)
                  //    ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetStyle().Colors[ImGuiCol_TextDisabled]);

                  const bool nodeOpen = ImGui::TreeNodeEx("", treeFlags, "%s", (const char*)ConvertString(obj->GetName()));

                  // if (node->DataMyBool == false)
                  //    ImGui::PopStyleColor();

                  if (SelectedObject != nullptr && ImGui::IsItemFocused())
                  {
                     *SelectedObject = obj;
                  }

                  if (nodeOpen)
                  {
                     ImGui::Text("Inner text");
                     ImGui::TreePop();
                  }
               }
            }
            ImGui::EndTable();
         }
      }
      ImGui::EndChild();

      // Right side: draw properties
      ImGui::SameLine();

      ImGui::BeginGroup(); // Lock X position

      if (UObject* obj = (SelectedObject ? SelectedObject->Get() : nullptr))
      {
         TATImGui::TextUnformatted(obj->GetName());
         ImGui::TextDisabled("UID: 0x%08X", obj->GetUniqueID());
         ImGui::Separator();
         if (ImGui::BeginTable("##properties", 2, ImGuiTableFlags_Resizable | ImGuiTableFlags_ScrollY))
         {
            {
               // Push object ID after we entered the table, so table is shared for all objects
               TATImGui::FScopedID tableId{ obj };

               ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthFixed);
               ImGui::TableSetupColumn("", ImGuiTableColumnFlags_WidthStretch, 2.0f); // Default twice larger
               {
                  // In a typical application, the structure description would be derived from a data-driven system.
                  // - We try to mimic this with our ExampleMemberInfo structure and the FObjectTreeNodeMemberInfos[] array.
                  // - Limits and some details are hard-coded to simplify the demo.
                  ForEachUObjectProperty(obj, [this](FName propName, const FProperty* prop, void* valuePtr)
                  {
                     if (!IsSupportedProperty(prop) || (IsPropertyVisible && !IsPropertyVisible(prop)))
                     {
                        return;
                     }

                     constexpr bool showLabel = false;

                     ImGui::TableNextRow();
                     TATImGui::FScopedID rowId{ prop };
                     ImGui::TableNextColumn();
                     ImGui::AlignTextToFramePadding();
                     TATImGui::TextUnformatted(propName.ToString());
                     ImGui::TableNextColumn();
                     ImGui::SetNextItemWidth(-FLT_MIN);
                     TATImGui::InputProperty(prop, valuePtr, showLabel);
                  });
               }
            }
            ImGui::EndTable();
         }
      }

      ImGui::EndGroup();
   }
}

#endif // TAT_ENABLE_DEV_TOOLS
