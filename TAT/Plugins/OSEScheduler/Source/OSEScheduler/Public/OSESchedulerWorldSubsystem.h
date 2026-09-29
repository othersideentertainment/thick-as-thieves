// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "OSESchedulerWorldSubsystem.generated.h"


class FOSESchedulerTaskSet;
DECLARE_DELEGATE_RetVal(TSharedRef<FOSESchedulerTaskSet>, FOSESchedulerCreateTaskSet)

UCLASS()
class OSESCHEDULER_API UOSESchedulerWorldSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   void CreateOrAddScheduledTask(
      const FName scheduleGroup,
      UActorComponent& component,
      const TFunctionRef<TSharedRef<FOSESchedulerTaskSet> ()>& createTaskSet
   );
   void RemoveScheduledTask(const FName scheduleGroup, UActorComponent& component);
   
   virtual void Deinitialize() override;
private:
   TMap<FName, TSharedRef<FOSESchedulerTaskSet>> _TaskSets;
};
