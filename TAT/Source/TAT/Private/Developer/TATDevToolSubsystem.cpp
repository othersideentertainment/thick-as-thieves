// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Developer/TATDevToolSubsystem.h"

// imgui
#if TAT_ENABLE_DEV_TOOLS
#include "imgui.h"
#include "imgui_internal.h"
#endif

// tat
#include "Developer/TATDevToolInterface.h"
#include "Developer/TATImGuiHelpers.h"

// ue
#include "EngineUtils.h"
#include "Developer/TATEditorSettings.h"
#include "Settings/TATGameUserSettings.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATDevToolSubsystem)

namespace DevToolHelpers
{
#if TAT_ENABLE_DEV_TOOLS
   struct FSceneOutlinerDevTool : public FTATDevToolStateUserData
   {
      bool UseDistanceFilter = false;
      FFloatInterval DistanceRange = { 0.0f, 500.0f };
      bool HighlightSelectedActorInWorld = true;
      FString ActorFilter;
      FString PropertyFilter;
      bool ShowUnsupportedProperties = false;
      TWeakObjectPtr<UActorComponent> EditingComponent;

      struct FPropEntry
      {
         FName PropName;
         const FProperty* Prop = nullptr;
         void* ValuePtr = nullptr;
      };
      TArray<FPropEntry> FilteredPropertyListCache;

      virtual ~FSceneOutlinerDevTool() override = default;

      void UpdateFilteredPropertyListCache(UObject* obj)
      {
         FilteredPropertyListCache.Reset();
         if (!IsValid(obj))
         {
            return;
         }
         TATImGui::ForEachUObjectProperty(obj, [this](FName propName, const FProperty* prop, void* valuePtr)
         {
            if (IsPropertyVisible(prop))
            {
               FilteredPropertyListCache.Emplace(propName, prop, valuePtr);
            }
         });
      }

      bool IsPropertyVisible(const FProperty* prop) const
      {
         if (prop == nullptr)
         {
            return false;
         }
         if (!ShowUnsupportedProperties && !TATImGui::IsSupportedProperty(prop))
         {
            return false;
         }
         if (!PropertyFilter.IsEmpty() && !prop->GetName().Contains(PropertyFilter, ESearchCase::IgnoreCase))
         {
            return false;
         }
         return true;
      }

      static bool InputProperty(FName propName, const FProperty* prop, void* valuePtr, UObject* parent)
      {
         bool modified = false;
         TATImGui::FScopedID propId{ prop };
         constexpr bool showLabel = false;
         TATImGui::NextProperty(propName);
         if (TATImGui::IsSupportedProperty(prop))
         {
            ImGui::BeginDisabled();
            //ImGui::PushItemFlag(ImGuiItemFlags_Disabled, true);
            modified = TATImGui::InputProperty(prop, valuePtr, showLabel);
            //ImGui::PopItemFlag();
            ImGui::EndDisabled();
         }
         else
         {
            FString str;
            if (prop->ExportText_Direct(str, valuePtr, nullptr, parent, 0))
            {
               TATImGui::TextUnformatted(str);
            }
            else
            {
               ImGui::Dummy({ 1, 1 });
            }
         }
         return modified;
      }

      static bool SelectableObject(UObject* obj, bool selected)
      {
         if (obj == nullptr)
         {
            return false;
         }
         return ImGui::Selectable(TATImGui::ConvertString(obj->GetName()), selected);
      }

      void DrawActorComponentTree(AActor* actor, UActorComponent* component = nullptr)
      {
         check(actor != nullptr);
         if (component == nullptr)
         {
            component = actor->GetRootComponent();
         }
         if (component == nullptr)
         {
            return;
         }

         // Draw this component entry
         if (SelectableObject(component, EditingComponent == component))
         {
            EditingComponent = component;
         }

         // Indent and draw components
         if (UPrimitiveComponent* primitiveComponent = Cast<UPrimitiveComponent>(component))
         {
            const int32 numChildren = primitiveComponent->GetNumChildrenComponents();
            if (numChildren > 0)
            {
               ImGui::Indent();
               for (int32 i = 0; i < primitiveComponent->GetNumChildrenComponents(); i++)
               {
                  ImGui::PushID(i);
                  DrawActorComponentTree(actor, primitiveComponent->GetChildComponent(i));
                  ImGui::PopID();
               }
               ImGui::Unindent();
            }
         }
      }

      void Draw(const FTATDevToolContext& context, FTATDevToolState& state)
      {
         const bool isOpen = TATImGui::BeginDevToolWindow(state);
         if (!isOpen)
         {
            return;
         }

         static const TCHAR* columnNames[] = { TEXT("Actor"), TEXT("Properties") };
         if (TATImGui::BeginColumnGroup("##Actors", columnNames))
         {
            if (TATImGui::NextColumnGroupColumn())
            {
               TATImGui::InputString(TEXT("Filter"), ActorFilter);

               ImGui::Checkbox("Filter by Distance", &UseDistanceFilter);
               if (UseDistanceFilter)
               {
                  ImGui::Indent();
                  ImGui::DragFloatRange2("Distance Range", &DistanceRange.Min, &DistanceRange.Max, 10.0f, 0.0f, std::numeric_limits<float>::max(), "%.2f");
                  ImGui::Unindent();
               }

               ImGui::Checkbox("Highlight Selected", &HighlightSelectedActorInWorld);
               ImGui::SetItemTooltip("Shows the bounding box of the selected actor in the world (if the actor has one)");

               if (ImGui::BeginChild("##ActorList", {}, ImGuiChildFlags_FrameStyle))
               {
                  const FFloatInterval distanceRangeSquared{ FMath::Square(DistanceRange.Min), FMath::Square(DistanceRange.Max) };
                  FVector distanceOrigin = FVector::ZeroVector;

                  check(context.World != nullptr);
                  if (APlayerController* pc = context.World->GetFirstPlayerController())
                  {
                     if (APawn* pawn = pc->GetPawn())
                     {
                        distanceOrigin = pawn->GetActorLocation();
                     }
                  }

                  for (auto it = TActorIterator<AActor>(context.World); it; ++it)
                  {
                     if (AActor* actor = *it)
                     {
                        // If the distance filter is enabled, only show actors near the player's pawn and ignore actors without a root component
                        if (UseDistanceFilter && (actor->GetRootComponent() == nullptr
                           || !distanceRangeSquared.Contains(FVector::DistSquared(distanceOrigin, actor->GetActorLocation()))))
                        {
                           continue;
                        }

                        // Name filter
                        if (!ActorFilter.IsEmpty() && !actor->GetName().Contains(ActorFilter, ESearchCase::IgnoreCase))
                        {
                           continue;
                        }

                        TATImGui::FScopedID _id{ actor };

                        const bool selected = actor == state.SelectedObject;
                        if (ImGui::Selectable(TATImGui::ConvertString(actor->GetName()), selected))
                        {
                           state.SelectedObject = actor;
                           EditingComponent = nullptr;
                        }
                     }
                  }
               }
               ImGui::EndChild();
            }

            AActor* selectedActor = Cast<AActor>(state.SelectedObject.Get());
            if (TATImGui::NextColumnGroupColumn() && IsValid(selectedActor))
            {
               TATImGui::FScopedID propEditorId{ selectedActor };

               const FVector actorLocation = selectedActor->GetActorLocation();
               FVector actorExtentOrigin = actorLocation;
               FVector actorExtent = FVector::ZeroVector;
               const bool actorHasTransform = selectedActor->GetRootComponent() != nullptr;
               if (actorHasTransform)
               {
                  selectedActor->GetActorBounds(false, actorExtentOrigin, actorExtent);
               }

               if (ImGui::BeginChild("##ActorComponentList", { 0.0f, 120.0f }, ImGuiChildFlags_FrameStyle | ImGuiChildFlags_ResizeY))
               {
                  ImGui::SeparatorText("Actor");

                  const bool isActorSelected = EditingComponent.IsExplicitlyNull();
                  if (SelectableObject(selectedActor, isActorSelected))
                  {
                     EditingComponent = nullptr;
                  }

                  ImGui::SeparatorText("Actor Components");

                  // Show basic actor components that don't have transforms first
                  selectedActor->ForEachComponent(false, [this](UActorComponent* component)
                  {
                     if (Cast<UPrimitiveComponent>(component) == nullptr)
                     {
                        ImGui::PushID(component);
                        if (SelectableObject(component, EditingComponent == component))
                        {
                           EditingComponent = component;
                        }
                        ImGui::PopID();
                     }
                  });

                  ImGui::SeparatorText("Scene Component Tree");

                  // Now show the component tree
                  DrawActorComponentTree(selectedActor);
               }
               ImGui::EndChild();

               TATImGui::InputString(TEXT("Property Filter"), PropertyFilter);
               ImGui::SameLine();
               ImGui::Checkbox("Show Unsupported", &ShowUnsupportedProperties);

               if (ImGui::BeginChild("##ActorPropertyEditor", {}, ImGuiChildFlags_Borders))
               {
                  if (EditingComponent.IsExplicitlyNull())
                  {
                     if (TATImGui::BeginPropertyEditor("##ActorProps"))
                     {
                        if (actorHasTransform && TATImGui::NextProperty(TEXT("Transform")))
                        {
                           ImGui::BeginDisabled();
                           FTransform transform = selectedActor->GetActorTransform();
                           TATImGui::InputTransform(TEXT("##transform"), transform);
                           ImGui::EndDisabled();
                        }

                        TATImGui::ForEachUObjectProperty(selectedActor, [&](FName propName, const FProperty* prop, void* valuePtr)
                        {
                           if (!IsPropertyVisible(prop))
                           {
                              return;
                           }
                           InputProperty(propName, prop, valuePtr, selectedActor);
                        });
                        TATImGui::EndPropertyEditor();
                     }
                  }
                  else if (UActorComponent* component = EditingComponent.Get())
                  {
                     TATImGui::FScopedID componentId{ component };
                     if (TATImGui::BeginPropertyEditor("##ComponentProps"))
                     {
                        TATImGui::ForEachUObjectProperty(component, [&](FName propName, const FProperty* prop, void* valuePtr)
                        {
                           if (!IsPropertyVisible(prop))
                           {
                              return;
                           }
                           InputProperty(propName, prop, valuePtr, component);
                        });
                        TATImGui::EndPropertyEditor();
                     }
                  }
               }
               ImGui::EndChild();

               if (HighlightSelectedActorInWorld && actorHasTransform)
               {
                  constexpr float pointSize = 15.0f;
                  DrawDebugPoint(context.World, actorLocation, pointSize, FColor::White);
                  if (FMath::Max3(actorExtent.X, actorExtent.Y, actorExtent.Z) > pointSize * 1.5f)
                  {
                     DrawDebugBox(context.World, actorExtentOrigin, actorExtent, FColor::White);
                  }
               }
            }

            TATImGui::EndColumnGroup();
         }

         TATImGui::EndDevToolWindow();
      }
   };
#endif // TAT_ENABLE_DEV_TOOLS

   bool IsClassDefaultVisible(UClass* cls)
   {
      check(IsValid(cls));
      for (const TSoftClassPtr<UObject>& defaultVisibleClass : UTATEditorSettings::Get().DevToolUISettings.DefaultVisibleClasses)
      {
         // No need to load the soft class pointer - if this class is one we want to match on, then it's already loaded
         UClass* thisClass = defaultVisibleClass.Get();
         if (thisClass != nullptr && thisClass->IsChildOf(cls))
         {
            return true;
         }
      }
      return false;
   }
}

bool UTATDevToolSubsystem::ShouldCreateSubsystem(UObject* outer) const
{
   if constexpr (EnableDevTools)
   {
      return Super::ShouldCreateSubsystem(outer);
   }
   else
   {
      return false;
   }
}

void UTATDevToolSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
   Super::Initialize(collection);

#if TAT_ENABLE_DEV_TOOLS
   // Cache the tool types that should be visible by default in TSets for quick lookup
   const FTATDevToolSubsystemSettings& settings =  UTATEditorSettings::Get().DevToolUISettings;
   for (const FString& name : settings.DefaultVisibleWindows)
   {
      _defaultVisibleWindowNames.Add(name);
   }

   if (settings.ShowImGuiDemoTool)
   {
      RegisterDevToolFunction(TEXT("ImGui Demo"), [](const FTATDevToolContext& context, FTATDevToolState& state)
      {
         ImGui::ShowDemoWindow(&state.IsOpen);
      });
   }

   RegisterDevToolFunction(TEXT("Scene Outliner"), [](const FTATDevToolContext& context, FTATDevToolState& state)
   {
      state.GetUserDataChecked<DevToolHelpers::FSceneOutlinerDevTool>().Draw(context, state);
   }, nullptr, MakeShared<DevToolHelpers::FSceneOutlinerDevTool>());

#endif // TAT_ENABLE_DEV_TOOLS
}

void UTATDevToolSubsystem::OnWorldBeginPlay(UWorld& inWorld)
{
   Super::OnWorldBeginPlay(inWorld);

   if constexpr (EnableDevTools)
   {
      APlayerController* pc = inWorld.GetFirstPlayerController();
      if (pc != nullptr && pc->InputComponent != nullptr)
      {
         static const FName toggleDevToolUIInputAction = FName(TEXT("ToggleDevToolUI"));
         pc->InputComponent->BindAction(toggleDevToolUIInputAction, IE_Pressed, this, &UTATDevToolSubsystem::_ToggleDevToolUI);
      }
   }
}

bool UTATDevToolSubsystem::IsTickable() const
{
   if constexpr (EnableDevTools)
   {
      return UTATEditorSettings::Get().EnableDevToolUI;
   }
   else
   {
      return false;
   }
}

void UTATDevToolSubsystem::Tick(float deltaTime)
{
   Super::Tick(deltaTime);

#if TAT_ENABLE_DEV_TOOLS

   if (!UTATEditorSettings::Get().EnableDevToolUI)
   {
      return;
   }

   int32 numDevToolsWithExpiredOwners = 0;

   const ImGui::FScopedContext ctx;
   if (ctx)
   {
      _DrawLaunchMenu();

      UWorld* world = GetWorld();

      for (int32 i = 0; i < _devTools.Num(); i++)
      {
         FRegisteredDevTool& tool = _devTools[i];

         // If we had an owner but no longer do, skip it and trigger a cleanup after we're done this loop
         if (tool.IsOwnerExpired())
         {
            ++numDevToolsWithExpiredOwners;
            continue;
         }

         if (!tool.State.IsOpen)
         {
            continue;
         }

         ImGui::PushID(i);

         // If we have a callback function, let that handle drawing the tool
         if (tool.Callback)
         {
            tool.Callback(
               FTATDevToolContext{ .DeltaSeconds = deltaTime, .World = world, .Owner = tool.Owner.Get() },
               tool.State);
         }
         else if (tool.ObjectCollectionName != NAME_None)
         {
            _DrawObjectCollectionDevTool(deltaTime, tool);
         }

         ImGui::PopID();
      }
   }

   // Remove all dev tools with an expired owner
   if (numDevToolsWithExpiredOwners > 0)
   {
      _devTools.RemoveAll([](const FRegisteredDevTool& tool) { return tool.IsOwnerExpired(); });
   }

#endif // TAT_ENABLE_DEV_TOOLS
}

int32 UTATDevToolSubsystem::RegisterDevToolFunction(FString&& label, FDevToolCallback&& callback, UObject* owner, const TSharedPtr<FTATDevToolStateUserData>& userData)
{
#if TAT_ENABLE_DEV_TOOLS
   if (!callback)
   {
      return INDEX_NONE;
   }
   return _RegisterDevToolInternal(FRegisteredDevTool(MoveTemp(label), MoveTemp(callback), owner, userData));
#else
   return INDEX_NONE;
#endif
}

void UTATDevToolSubsystem::UnregisterDevToolFunction(int32 devToolId)
{
#if TAT_ENABLE_DEV_TOOLS
   const int32 idx = _devTools.IndexOfByPredicate([devToolId](const FRegisteredDevTool& state) { return state.Id == devToolId; });
   if (idx != INDEX_NONE)
   {
      _devTools.RemoveAt(idx);
   }
#endif
}

void UTATDevToolSubsystem::RegisterDevToolObject(UObject* obj, FName objectCollectionName)
{
#if TAT_ENABLE_DEV_TOOLS
   if (!IsValid(obj))
   {
      return;
   }

   if (objectCollectionName == NAME_None)
   {
      objectCollectionName = obj->GetClass()->GetFName();
   }
   if (!ensure(objectCollectionName != NAME_None))
   {
      return;
   }

   FDevToolObjectCollection* objectCollection = _objectCollections.Find(objectCollectionName);
   if (objectCollection == nullptr)
   {
      FRegisteredDevTool newDevTool{ objectCollectionName.ToString(), objectCollectionName };
      // Open by default if requested
      if (DevToolHelpers::IsClassDefaultVisible(obj->GetClass()))
      {
         newDevTool.State.IsOpen = true;
      }
      // Set the default selected object to the first object
      newDevTool.State.SelectedObject = obj;
      const int32 newId = _RegisterDevToolInternal(MoveTemp(newDevTool));

      // Make an object collection for this dev tool to store the list of registered objects we can view and edit
      FDevToolObjectCollection newCollection{};
      newCollection.Id = newId;
      check(newCollection.Id != INDEX_NONE);
      objectCollection = &_objectCollections.Add(objectCollectionName, MoveTemp(newCollection));
   }

   check(objectCollection != nullptr);
   if (!ensure(_devTools.ContainsByPredicate([id = objectCollection->Id](const FRegisteredDevTool& devTool) { return devTool.Id == id; })))
   {
      return;
   }

   objectCollection->Objects.AddUnique(obj);
   _objectToCollectionNameMap.Add(obj, objectCollectionName);
#endif // TAT_ENABLE_DEV_TOOLS
}

void UTATDevToolSubsystem::UnregisterDevToolObject(UObject* obj)
{
#if TAT_ENABLE_DEV_TOOLS
   if (obj == nullptr)
   {
      return;
   }

   TWeakObjectPtr<UObject> weakObj = obj;

   const FName* collectionNamePtr = _objectToCollectionNameMap.Find(weakObj);
   if (collectionNamePtr == nullptr)
   {
      return;
   }

   const FName collectionName = *collectionNamePtr;
   _objectToCollectionNameMap.Remove(weakObj);

   FDevToolObjectCollection* objectCollection = _objectCollections.Find(collectionName);
   if (objectCollection == nullptr)
   {
      return;
   }

   const int collectionId = objectCollection->Id;
   objectCollection->Objects.Remove(weakObj);

   if (objectCollection->Objects.IsEmpty())
   {
      _devTools.RemoveAll([id = collectionId](const FRegisteredDevTool& devTool) { return devTool.Id == id; });
      _objectCollections.Remove(collectionName);
   }
#endif // TAT_ENABLE_DEV_TOOLS
}

bool UTATDevToolSubsystem::ToggleDevToolByName(const FString& name)
{
#if TAT_ENABLE_DEV_TOOLS
   int32 numDevToolsEnabled = 0;
   int32 numDevToolsDisabled = 0;

   auto devToolMatchesQuery = [query = FStringView(name)](FStringView toolName) -> bool
   {
      return toolName.Contains(query, ESearchCase::IgnoreCase);
   };
   
   for (FRegisteredDevTool& tool : _devTools)
   {
      if (!devToolMatchesQuery(tool.State.Label))
      {
         continue;
      }
      
      if (tool.State.IsOpen)
      {
         ++numDevToolsDisabled;
      }
      else
      {
         ++numDevToolsEnabled;
      }

      tool.State.IsOpen = !tool.State.IsOpen;
   }

   // Toggle dev tool rendering if we just enabled one and they were previously disabled
   if (numDevToolsEnabled > 0 && !UTATEditorSettings::Get().EnableDevToolUI)
   {
      _ToggleDevToolUI();
   }

   return numDevToolsEnabled > 0 || numDevToolsDisabled > 0;
#else
   return false;
#endif // TAT_ENABLE_DEV_TOOLS
}

void UTATDevToolSubsystem::GetAllDevToolNames(TArray<FString>& outNames) const
{
#if TAT_ENABLE_DEV_TOOLS
   outNames.SetNum(_devTools.Num());
   for (int32 i = 0; i < _devTools.Num(); ++i)
   {
      outNames[i] = _devTools[i].State.Label;
   }
#endif // TAT_ENABLE_DEV_TOOLS
}

void UTATDevToolSubsystem::_ToggleDevToolUI()
{
   check(EnableDevTools);
   UTATEditorSettings& editorSettings = UTATEditorSettings::GetMutable();
   editorSettings.EnableDevToolUI = !editorSettings.EnableDevToolUI;
}

#if TAT_ENABLE_DEV_TOOLS

int32 UTATDevToolSubsystem::_RegisterDevToolInternal(FRegisteredDevTool&& state)
{
   check(state.Id == INDEX_NONE);
   const int32 thisId = _nextDevToolId;
   ++_nextDevToolId;
   state.Id = thisId;

   // Make sure we always have a valid window title
   if (state.State.Label.IsEmpty())
   {
      state.State.Label = FString::Printf(TEXT("Untitled Dev Tool %i"), state.Id);
   }

   // Check if the user wants this window to be visible by default (either by name or by owner class)
   if (_defaultVisibleWindowNames.Contains(state.State.Label))
   {
      state.State.IsOpen = true;
   }
   else if (const UObject* owner = state.Owner.Get())
   {
      if (IsValid(owner) && DevToolHelpers::IsClassDefaultVisible(owner->GetClass()))
      {
         state.State.IsOpen = true;
      }
   }

   _devTools.Add(MoveTemp(state));
   return thisId;
}

void UTATDevToolSubsystem::_DrawLaunchMenu()
{
   ImGuiIO& io = ImGui::GetIO();
   ImGuiWindowFlags windowFlags =
      ImGuiWindowFlags_NoDecoration
      | ImGuiWindowFlags_NoDocking
      | ImGuiWindowFlags_AlwaysAutoResize
      | ImGuiWindowFlags_NoSavedSettings
      | ImGuiWindowFlags_NoFocusOnAppearing
      | ImGuiWindowFlags_NoNav;

   const FTATDevToolSubsystemSettings& settings = UTATEditorSettings::Get().DevToolUISettings;

   if (settings.MenuPos == ETATDevMenuPos::Custom)
   {
      ImGui::SetNextWindowPos(settings.CustomMenuPos, ImGuiCond_Appearing);
   }
   else
   {
      constexpr float padding = 10.0f;
      const ImGuiViewport* viewport = ImGui::GetMainViewport();
      const ImVec2 workPos = viewport->WorkPos; // Use work area to avoid menu-bar/task-bar, if any!
      const ImVec2 workSize = viewport->WorkSize;
      ImVec2 windowPos;
      ImVec2 windowPosPivot;

      switch (settings.MenuPos)
      {
      case ETATDevMenuPos::TopLeft:
      case ETATDevMenuPos::BottomLeft:
         windowPos.x = workPos.x + padding;
         windowPosPivot.x = 0.0f;
         break;
      case ETATDevMenuPos::TopCenter:
      case ETATDevMenuPos::BottomCenter:
         windowPos.x = workPos.x + (workSize.x * 0.5f) - padding;
         windowPosPivot.x = 0.5f;
         break;
      case ETATDevMenuPos::TopRight:
      case ETATDevMenuPos::BottomRight:
         windowPos.x = workPos.x + workSize.x - padding;
         windowPosPivot.x = 1.0f;
         break;
      default:
         break;
      }

      switch (settings.MenuPos)
      {
      case ETATDevMenuPos::TopLeft:
      case ETATDevMenuPos::TopCenter:
      case ETATDevMenuPos::TopRight:
         windowPos.y = workPos.y + padding;
         windowPosPivot.y = 0.0f;
         break;
      case ETATDevMenuPos::BottomLeft:
      case ETATDevMenuPos::BottomCenter:
      case ETATDevMenuPos::BottomRight:
         windowPos.y = workPos.y + workSize.y - padding;
         windowPosPivot.y = 1.0f;
         break;
      default:
         break;
      }

      ImGui::SetNextWindowPos(windowPos, ImGuiCond_Always, windowPosPivot);
      ImGui::SetNextWindowViewport(viewport->ID);
      windowFlags |= ImGuiWindowFlags_NoMove;
   }

   FVector2D currentWindowPos = FVector2D::ZeroVector;

   auto setMenuPos = [this, &currentWindowPos](ETATDevMenuPos newMenuPos)
   {
      FTATDevToolSubsystemSettings& mutableSettings = UTATEditorSettings::GetMutable().DevToolUISettings;
      mutableSettings.MenuPos = newMenuPos;
      mutableSettings.CustomMenuPos = (mutableSettings.MenuPos == ETATDevMenuPos::Custom) ? currentWindowPos : FVector2D::ZeroVector;
   };

   ImGui::SetNextWindowBgAlpha(0.5f); // Transparent background
   if (ImGui::Begin("Dev Tools", nullptr, windowFlags))
   {
      currentWindowPos = ImGui::GetWindowPos();

      if (ImGui::Button("Dev Tools")) { ImGui::OpenPopup("##DevToolsPopup"); }
      ImGui::SameLine();
      if (ImGui::SmallButton("...")) { ImGui::OpenPopup("##DevToolsSettingsPopup"); }

      if (ImGui::BeginPopup("##DevToolsPopup"))
      {
         for (int32 i = 0; i < _devTools.Num(); i++)
         {
            ImGui::PushID(i);

            FRegisteredDevTool& tool = _devTools[i];

            ImGui::MenuItem(TATImGui::ConvertString(tool.State.Label), nullptr, &tool.State.IsOpen);

            if (ImGui::BeginPopupContextWindow())
            {
               bool defaultVisible = _defaultVisibleWindowNames.Contains(tool.State.Label);
               if (ImGui::MenuItem("Auto-Open", nullptr, &defaultVisible))
               {
                  if (defaultVisible)
                  {
                     _defaultVisibleWindowNames.Add(tool.State.Label);
                     UTATEditorSettings::GetMutable().DevToolUISettings.DefaultVisibleWindows.AddUnique(tool.State.Label);
                  }
                  else
                  {
                     _defaultVisibleWindowNames.Remove(tool.State.Label);
                     UTATEditorSettings::GetMutable().DevToolUISettings.DefaultVisibleWindows.Remove(tool.State.Label);
                  }
               }
               ImGui::EndPopup();
            }

            ImGui::PopID();
         }
         ImGui::EndPopup();
      }

      if (ImGui::BeginPopup("##DevToolsSettingsPopup"))
      {
         if (ImGui::MenuItem("Top-left", nullptr, settings.MenuPos == ETATDevMenuPos::TopLeft)) { setMenuPos(ETATDevMenuPos::TopLeft); }
         if (ImGui::MenuItem("Top-center", nullptr, settings.MenuPos == ETATDevMenuPos::TopCenter)) { setMenuPos(ETATDevMenuPos::TopCenter); }
         if (ImGui::MenuItem("Top-right", nullptr, settings.MenuPos == ETATDevMenuPos::TopRight)) { setMenuPos(ETATDevMenuPos::TopRight); }
         if (ImGui::MenuItem("Bottom-left", nullptr, settings.MenuPos == ETATDevMenuPos::BottomLeft)) { setMenuPos(ETATDevMenuPos::BottomLeft); }
         if (ImGui::MenuItem("Bottom-center", nullptr, settings.MenuPos == ETATDevMenuPos::BottomCenter)) { setMenuPos(ETATDevMenuPos::BottomCenter); }
         if (ImGui::MenuItem("Bottom-right", nullptr, settings.MenuPos == ETATDevMenuPos::BottomRight)) { setMenuPos(ETATDevMenuPos::BottomRight); }
         if (ImGui::MenuItem("Custom Position", nullptr, settings.MenuPos == ETATDevMenuPos::Custom)) { setMenuPos(ETATDevMenuPos::Custom); }
         ImGui::Separator();
         if (ImGui::MenuItem("Hidden")) { UTATEditorSettings::GetMutable().EnableDevToolUI = false; }
         if (ImGui::IsItemHovered())
         {
#if WITH_EDITOR
            ImGui::SetTooltip("Press F10 to toggle visibility, or re-enable in per-user editor settings");
#else
            ImGui::SetTooltip("Press F10 to toggle visibility");
#endif
         }
         ImGui::EndPopup();
      }
   }
   ImGui::End();

   if (settings.MenuPos == ETATDevMenuPos::Custom && currentWindowPos != settings.CustomMenuPos)
   {
      UTATEditorSettings::GetMutable().DevToolUISettings.CustomMenuPos = currentWindowPos;
   }
}

void UTATDevToolSubsystem::_DrawObjectCollectionDevTool(float deltaSeconds, FRegisteredDevTool& devTool)
{
   auto getObjectName = [](const UObject* obj) -> FString
   {
      FString result{};
      if (IsValid(obj))
      {
         if (const UActorComponent* actorComp = Cast<UActorComponent>(obj))
         {
            result = FString::Printf(TEXT("%s.%s"), *GetNameSafe(actorComp->GetOwner()), *GetNameSafe(obj));
         }
         else
         {
            result = GetNameSafe(obj);
         }
         // Let the object customize the name if desired
         if (const ITATDevToolInterface* objectDevToolInterface = Cast<ITATDevToolInterface>(obj))
         {
            objectDevToolInterface->GetDevToolObjectName(result);
         }
      }
      else
      {
         result = TEXT("INVALID");
      }
      return result;
   };

   FDevToolObjectCollection* objectCollection = _objectCollections.Find(devTool.ObjectCollectionName);
   if (objectCollection == nullptr)
   {
      return;
   }

   ImGui::SetNextWindowSize({ 350, 200 }, ImGuiCond_FirstUseEver);
   if (TATImGui::BeginDevToolWindow(devTool.State))
   {
      if (ImGui::BeginTable("##objects", 2, ImGuiTableFlags_Resizable, ImGui::GetContentRegionAvail()))
      {
         ImGui::TableSetupColumn("Objects");
         ImGui::TableSetupColumn("Properties");

         UObject* selectedObject = devTool.State.SelectedObject.Get();

         if (ImGui::TableNextColumn())
         {
            if (ImGui::BeginChild("##ObjectList", {}, ImGuiChildFlags_FrameStyle))
            {
               for (const TWeakObjectPtr<UObject>& weakObj : objectCollection->Objects)
               {
                  if (UObject* obj = weakObj.Get())
                  {
                     ImGui::PushID(obj);

                     const bool isSelected = obj == selectedObject;
                     if (ImGui::Selectable(TATImGui::ConvertString(getObjectName(obj)), isSelected))
                     {
                        selectedObject = obj;
                        devTool.State.SelectedObject = selectedObject;
                     }

                     ImGui::PopID();
                  }
               }
            }
            ImGui::EndChild();
         }

         if (ImGui::TableNextColumn() && IsValid(selectedObject))
         {
            ImGui::PushID(selectedObject);

            if (ImGui::BeginChild("##Props"))
            {
               if (ITATDevToolInterface* objectDevToolInterface = Cast<ITATDevToolInterface>(selectedObject))
               {
                  objectDevToolInterface->DrawDevToolObjectEditor(deltaSeconds);
               }
               else
               {
                  TATImGui::ForEachUObjectProperty(selectedObject, [](FName propName, const FProperty* prop, void* valuePtr)
                  {
                     if (TATImGui::IsSupportedProperty(prop))
                     {
                        TATImGui::InputProperty(prop, valuePtr);
                     }
                     else
                     {
                        TATImGui::Text(TEXT("%s"), *propName.ToString());
                     }
                  });
               }
            }
            ImGui::EndChild();

            ImGui::PopID();
         }

         ImGui::EndTable();
      }

      TATImGui::EndDevToolWindow();
   }
}

#endif // TAT_ENABLE_DEV_TOOLS
