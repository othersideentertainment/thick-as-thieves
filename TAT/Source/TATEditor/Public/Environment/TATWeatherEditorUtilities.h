// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "IDetailsView.h"

class USCS_Node;

namespace WeatherEditorUtilities
{
   /// If there is exactly one actor selected in the editor, returns it. Otherwise returns null.
   AActor* GetEditorSelectedActor();

   /// Returns the blueprint asset that represents a class, if any.
   UBlueprint* FindBlueprintAssetForClass(UClass* cls);

   /// Sets the content browser selection to the blueprint asset represented by the specified class.
   bool FindAssetInContentBrowser(UClass* cls);

   /// Opens the blueprint editor for the asset represented by the specified class.
   bool OpenAssetEditorForClass(UClass* cls);

   /// Opens the data table editor
   bool OpenAssetEditorForDataTable(UDataTable* dataTable);

   /// Checks if a class is a blueprint class that has unsaved changes
   bool IsBlueprintClassAssetDirty(UClass* cls);

   /// Compiles a blueprint class
   void CompileBlueprintClassAsset(UClass* cls);

   /// If the class is a blueprint class with unsaved changes, saves it.
   /// Prompts the user to checkout the file first if needed.
   /// Returns true if the asset was successfully saved.
   bool SaveBlueprintClassAsset(UClass* cls);

   /// Gets a property value on any UObject as a string
   bool GetPropertyValueAsString(UObject* obj, FName propName, FString& outPropValueString);

   /// Sets a property value on any UObject from a string value
   bool SetPropertyValueAsString(UObject* obj, FName propName, const FString& newPropValue, FString* outErrorMessage = nullptr);

   /// Copies all properties from one component to another
   int32 CopyComponentProps(UActorComponent* targetComponent, UActorComponent* sourceComponent, TFunctionRef<bool(const FProperty*)> shouldCopyProp);

   /// Finds the first component of a type on a source actor, then finds the first component of the same type on another actor's CDO,
   /// then copies all properties from the source actor's component to the CDO's component template.
   /// If the destination class is a blueprint asset, marks it as dirty (eg. has unsaved changes).
   int32 CopyComponentPropsToCDO(TSubclassOf<UActorComponent> componentClass, TSubclassOf<AActor> destClass, AActor* sourceActor, TFunctionRef<bool(const FProperty*)> shouldCopyProp);

   /// Finds the first actor of the specified type in the editor world's persistent level
   /// If world is not specified or null, the current editor world will be used
   AActor* FindFirstActorOfTypeInPersistentLevel(UClass* actorClass, UWorld* world = nullptr);

   /// Finds the first actor of the specified type in the editor world's persistent level
   /// If world is not specified or null, the current editor world will be used
   template<typename T>
   FORCEINLINE static T* FindFirstActorOfTypeInPersistentLevel(UWorld* world = nullptr)
   {
      return Cast<T>(FindFirstActorOfTypeInPersistentLevel(T::StaticClass(), world));
   }

   /// Calls a callback for each component template in an actor's CDO.
   /// For components defined in native, the FObjectProperty* argument will be non-null and can be used to extract UPROPERTY metadata flags.
   /// For components defined in blueprints, the USCS_Node* argument will be non-null and can be used to extract blueprint metadata.
   void ForEachComponentInActorCDO(TSubclassOf<AActor> actorClass, TFunctionRef<void(UActorComponent*, const FObjectProperty*, USCS_Node*)> callback);

   /// Makes a human readable label for the specified class
   FText MakeBlueprintAssetLabel(UClass* cls);

   /// Makes a human readable label for the specified class (soft object path version that does _not_ resolve the object, just extracts the name from the path)
   FText MakeBlueprintAssetLabel(const FSoftObjectPath& path);

   /// Returns a slate brush with an icon representing the specified class. Works for most builtin component and actor types
   /// (eg. DirectionalLightComponent returns the directional light icon)
   const FSlateBrush* GetIconBrush(TSubclassOf<UObject> cls);

   /// Constructs a text block with the FontAwesome font using the specified font awesome codepoint (or preconstructed FText containing that codepoint)
   /// See also: FEditorFontGlyphs and WeatherIconGlyph
   TSharedRef<SWidget> MakeFontAwesomeIcon(const FText& icon, const TOptional<FText>& tooltipText = NullOpt, const TOptional<FLinearColor>& iconColor = NullOpt);
   TSharedRef<SWidget> MakeFontAwesomeIcon(TCHAR codepoint, const TOptional<FText>& tooltipText = NullOpt, const TOptional<FLinearColor>& iconColor = NullOpt);

   /// Makes a button widget that calls the callback when clicked
   TSharedRef<SWidget> MakeSimpleButton(const FText& label, const FText& tooltipText, const TFunction<void()>& onClickCallback, float margin = 2.0f);

   /// Makes a button widget containing a single icon that calls the callback when clicked.
   /// Use FEditorFontGlyphs or WeatherIconGlyph to specify the desired icon
   /// Example: MakeIconButton(WeatherIconGlyph::Fire(), FText::GetEmpty(), []() {})
   TSharedRef<SWidget> MakeIconButton(const FText& icon, const FText& tooltipText, const TFunction<void()>& onClickCallback, const TOptional<FLinearColor>& iconColor = NullOpt, float margin = 2.0f);
   TSharedRef<SWidget> MakeIconButton(TCHAR codepoint, const FText& tooltipText, const TFunction<void()>& onClickCallback, const TOptional<FLinearColor>& iconColor = NullOpt, float margin = 2.0f);

   /// Makes a checkbox with the value of the checked attribute
   TSharedRef<SWidget> MakeSimpleCheckBox(const FText& label, const FText& tooltipText, TAttribute<bool> isChecked, const TFunction<void(bool)>& onCheckStateChangeCallback);

   /// Makes a button that shows an eyeball or an eyeball with a slash through it depending on the visibility value
   TSharedRef<SWidget> MakeVisibilityButton(const FOnClicked& onClickDelegate, TAttribute<bool> isVisible);

   /// Shows a popup context menu at the cursor location
   /// You probably want to use FMenuBuilder to construct the context menu itself.
   void ShowContextMenuAtCursorLocation(const TSharedRef<SWidget>& parentWidget, const TSharedRef<SWidget>& contextMenu);

   /// Makes a general purpose property editor for an arbitrary object.
   TSharedRef<IDetailsView> MakePropertyEditorWidget(UObject* editingObject, const TOptional<FDetailsViewArgs>& args = NullOpt, const TOptional<FIsPropertyVisible>& propVisible = NullOpt);

   /// Manages state for a simple actor editor widget.
   /// Works similarly to the Unreal level editor one but is simpler and has much more flexibility WRT to the list of editable components.
   class FSimpleActorEditor
   {
      using PropertyChangedFn = TFunction<void(UObject*, const FPropertyChangedEvent&)>;

      TSubclassOf<AActor> _actorClass;
      FDetailsViewArgs _detailsViewArgs;
      TSharedPtr<SBox> _propertyEditorOuter;
      PropertyChangedFn _propertyChangedFn;

      struct FObjectEditorState
      {
         TSubclassOf<UObject> Cls;
         TWeakObjectPtr<UObject> Template;
         TFunction<bool()> IsObjectVisible;
         TOptional<FIsPropertyVisible> IsPropertyVisibleDelegate;
         bool IsSeparator = false;
         int32 IndentLevel = 0;
         FText NameLabel;
         FText TypeLabel;
      };
      TArray<TSharedPtr<FObjectEditorState>> _objectList;

      static TSharedPtr<FObjectEditorState> _MakeObjectEditorState(UObject* templateObject, const TOptional<FIsPropertyVisible>& isPropertyVisible, int32 indentLevel);

   public:
      FSimpleActorEditor();

      /// Clears the list of editable objects and sets up the data to edit a new actor class.
      /// The IsPropertyVisible delegate will be assigned to the actor object and to any components added if autoAddComponents is true.
      void SetActorClass(TSubclassOf<AActor> inActorClass, const TOptional<FIsPropertyVisible>& isPropertyVisible = NullOpt, bool autoAddComponents = true);

      /// Adds a component that will show up in the actor's component list and be editable.
      void AddActorComponentEditor(UActorComponent* componentTemplate, const TOptional<FIsPropertyVisible>& isPropertyVisible = NullOpt, const TOptional<TFunction<bool()>>& isComponentVisible = NullOpt, int32 indentLevel = 1);

      /// Sets a callback that will be executed when any actor or component property changes
      /// Must be called before Construct.
      void SetPropertyChangedCallback(const PropertyChangedFn& callback) { _propertyChangedFn = callback; }

      /// Returns a mutable reference to the property editor settings.
      /// Set these before calling SetActorClass
      FDetailsViewArgs& PropertyEditorViewArgs() { return _detailsViewArgs; }

      /// Builds and returns the actor editor widget
      TSharedRef<SWidget> Construct();

   private:
      void _SetPropertyEditor(const TSharedPtr<FObjectEditorState>& item);
   };
}

/// FontAwesome glyph codepoints for use with MakeFontAwesomeIcon and MakeIconButton
namespace WeatherIconGlyph
{
#define TAT_DEFINE_GLYPH(NAME, CODEPOINT) \
   static constexpr TCHAR NAME##_Codepoint = CODEPOINT; \
   inline FText NAME() \
   { \
      static constexpr TCHAR iconString[]{ CODEPOINT, TEXT('\0') }; \
      static const FText iconText = FText::FromString(iconString); \
      return iconText; \
   }

   TAT_DEFINE_GLYPH(Image, TEXT('\xf03e')); // https://fontawesome.com/icons/image
   TAT_DEFINE_GLYPH(Ban, TEXT('\xf05e')); // https://fontawesome.com/icons/ban
   TAT_DEFINE_GLYPH(Cube, TEXT('\xf1b2')); // https://fontawesome.com/icons/cube
   TAT_DEFINE_GLYPH(Star, TEXT('\xf005')); // https://fontawesome.com/icons/star

#undef TAT_DEFINE_GLYPH
}
