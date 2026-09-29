// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"

// tat
#include "Player/Perception/TATPlayerPerceivableComponent.h"

#include "TATPlayerPerceptionSubsystem.generated.h"

/// The subsystem responsible for negotiating the link between the perception component
/// and the perceivables in the level
UCLASS()
class TAT_API UTATPlayerPerceptionSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()
public:

   void RegisterPerceivable(UTATPlayerPerceivableComponent* perceivable);
   void UnregisterPerceivable(UTATPlayerPerceivableComponent* perceivable);

   const TArray<TWeakObjectPtr<UTATPlayerPerceivableComponent>>& GetPerceivables() const { return _perceivables; }

   DECLARE_EVENT_OneParam(UTATPlayerPerceptionSubsystem, FOnPerceivableRegistered, UTATPlayerPerceivableComponent*);
   FOnPerceivableRegistered OnPerceivableRegistered;

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLocalPerceptionLevelChangedForPerceivable, UTATPlayerPerceivableComponent*, perceivable, ETATPlayerPerceptionLevel, newPerceptionLevel);
   UPROPERTY(BlueprintAssignable)
   FOnLocalPerceptionLevelChangedForPerceivable OnLocalPerceptionLevelChangedForPerceivable;

private:
   TArray<TWeakObjectPtr<UTATPlayerPerceivableComponent>> _perceivables;
};

