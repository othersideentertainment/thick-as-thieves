// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "OSESchedulerTaskSet.generated.h"
class UOSESchedulerWorldSubsystem;

USTRUCT()
struct FOSESchedulerTaskSetTickFunction : public FTickFunction
{
   GENERATED_BODY()
   class FOSESchedulerTaskSet* Target { nullptr };
   // Begin FTickFunction Interface
   virtual void ExecuteTick(float deltaTime, ELevelTick tickType, ENamedThreads::Type currentThread, const FGraphEventRef& myCompletionGraphEvent) override;
   virtual FString DiagnosticMessage() override;
   virtual FName DiagnosticContext(bool bDetailed)  override;
   // End FTickFunction Interface
};

template<>
struct TStructOpsTypeTraits<FOSESchedulerTaskSetTickFunction> : public TStructOpsTypeTraitsBase2<FOSESchedulerTaskSetTickFunction>
{
   enum
   {
      WithCopy = false
   };
};

class OSESCHEDULER_API FOSESchedulerTaskSet : public TSharedFromThis<FOSESchedulerTaskSet>
{
public:
   FOSESchedulerTaskSet(const ETickingGroup tickingGroup, const UWorld* world);
   virtual ~FOSESchedulerTaskSet();
protected:
   FOSESchedulerTaskSetTickFunction _TickFunction;
   virtual void ExecuteTick(const float worldTime, const float deltaTime);
   virtual void TryExecuteTickOnComponent(UActorComponent* component, const float deltaTime);
   virtual void AddComponent(TWeakObjectPtr<UActorComponent> component) { _Components.Add(component); }

   TArray<TWeakObjectPtr<UActorComponent>> _Components;
   TWeakObjectPtr<const UWorld> _World;
   friend UOSESchedulerWorldSubsystem;
   friend FOSESchedulerTaskSetTickFunction;
};

class OSESCHEDULER_API FOSESchedulerTaskSet_MaxFrameTimeTick : public FOSESchedulerTaskSet
{
public:
   FOSESchedulerTaskSet_MaxFrameTimeTick(const ETickingGroup tickingGroup, const UWorld* world, const float maxFrameTime);
   
protected:
   virtual void ExecuteTick(const float worldTime, const float deltaTime) override;

   int IndexToTickIfBreaking { 0 };
   float MaxFrameTime { 1.f };
};

class OSESCHEDULER_API FOSESchedulerTaskSet_DelayedComponentTick : public FOSESchedulerTaskSet
{
public:
   FOSESchedulerTaskSet_DelayedComponentTick(const ETickingGroup tickingGroup, const UWorld* world, const float timeBetweenIndividualComponentTicks);
protected:
   virtual void ExecuteTick(const float worldTime, const float deltaTime) override;
   virtual void AddComponent(TWeakObjectPtr<UActorComponent> component) override;

   TMap<TWeakObjectPtr<UActorComponent>, float> MapOfComponentsToTimeLastTicked;
   float JitterOffsetForPreviouslyAddedComponent { 0.f };
   float TimeBetweenIndividualComponentTicks { 1.f };
};
