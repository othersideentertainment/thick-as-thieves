// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Developer/TATDevToolTypes.h"

// ue
#include "Subsystems/WorldSubsystem.h"

#include "TATDevToolSubsystem.generated.h"

struct FTATDevToolContext
{
   float DeltaSeconds = 0.0f;
   UWorld* World = nullptr;
   UObject* Owner = nullptr;
};

class FTATDevToolStateUserData
{
public:
   virtual ~FTATDevToolStateUserData() = default;
};

struct FTATDevToolState
{
   FString Label;
   bool IsOpen = false;

   // General purpose state that any callback can use for its own purposes
   TWeakObjectPtr<UObject> SelectedObject;
   int32 SelectedIndex = INDEX_NONE;
   TSharedPtr<FTATDevToolStateUserData> UserData;

   FTATDevToolState() = default;
   explicit FTATDevToolState(FString&& label) : Label(MoveTemp(label)) {}

   template<typename T>
   FORCEINLINE T& GetUserDataChecked() { TSharedPtr<T> ptr = StaticCastSharedPtr<T>(UserData); check(ptr != nullptr); return *ptr; }
   template<typename T>
   FORCEINLINE const T& GetUserDataChecked() const { return const_cast<FTATDevToolState*>(this)->GetUserDataChecked<T>(); }
};

UCLASS()
class TAT_API UTATDevToolSubsystem : public UTickableWorldSubsystem
{
   GENERATED_BODY()

public:
   using FDevToolCallback = TFunction<void(const FTATDevToolContext&, FTATDevToolState&)>;

#if TAT_ENABLE_DEV_TOOLS
   static constexpr bool EnableDevTools = true;
#else
   static constexpr bool EnableDevTools = false;
#endif

   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
   virtual void Initialize(FSubsystemCollectionBase& collection) override;
   virtual void OnWorldBeginPlay(UWorld& inWorld) override;
   virtual bool IsTickable() const override;
   virtual void Tick(float deltaTime) override;
   virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UTATDevToolSubsystem, STATGROUP_Tickables); }
   virtual bool IsTickableWhenPaused() const override { return EnableDevTools; }

   int32 RegisterDevToolFunction(FString&& label, FDevToolCallback&& callback, UObject* owner = nullptr, const TSharedPtr<FTATDevToolStateUserData>& userData = nullptr);
   void UnregisterDevToolFunction(int32 devToolId);

   void RegisterDevToolObject(UObject* obj, FName objectCollectionName = NAME_None);
   void UnregisterDevToolObject(UObject* obj);

   /// Toggles the visibility of a dev tool by name.
   /// Returns true if any matching dev tools were found and toggled.
   bool ToggleDevToolByName(const FString& name);

   /// Gets the name of all currently registered dev tools
   void GetAllDevToolNames(TArray<FString>& outNames) const;

private:
   UFUNCTION()
   void _ToggleDevToolUI();

#if TAT_ENABLE_DEV_TOOLS

   int32 _nextDevToolId = 1;

   struct FRegisteredDevTool
   {
      int32 Id = INDEX_NONE;
      FTATDevToolState State;
      FDevToolCallback Callback;
      TWeakObjectPtr<UObject> Owner;
      FName ObjectCollectionName = NAME_None;

      FRegisteredDevTool() = default;

      FRegisteredDevTool(FString&& label, FName objectCollectionName)
         : State(MoveTemp(label))
         , ObjectCollectionName(objectCollectionName)
      {
      }

      FRegisteredDevTool(FString&& label, FDevToolCallback&& callback, UObject* owner, const TSharedPtr<FTATDevToolStateUserData>& userData)
         : State(MoveTemp(label))
         , Callback(MoveTemp(callback))
         , Owner(owner)
      {
         State.UserData = userData;
      }

      FORCEINLINE bool IsOwnerExpired() const { return !Owner.IsExplicitlyNull() && Owner.IsStale(); }
   };
   TArray<FRegisteredDevTool> _devTools;

   struct FDevToolObjectCollection
   {
      int32 Id = INDEX_NONE;
      TArray<TWeakObjectPtr<UObject>> Objects;
   };
   TMap<FName, FDevToolObjectCollection> _objectCollections;
   TMap<TWeakObjectPtr<UObject>, FName> _objectToCollectionNameMap;

   TSet<FString> _defaultVisibleWindowNames;

   int32 _RegisterDevToolInternal(FRegisteredDevTool&& state);

   void _DrawLaunchMenu();

   void _DrawObjectCollectionDevTool(float deltaSeconds, FRegisteredDevTool& devTool);

#endif // TAT_ENABLE_DEV_TOOLS
};
