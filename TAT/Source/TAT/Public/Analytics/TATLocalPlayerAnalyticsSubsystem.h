// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "TATAnalyticsManager.h"

#include "Subsystems/LocalPlayerSubsystem.h"
#include "TATLocalPlayerAnalyticsSubsystem.generated.h"

struct FMatchPersistentData;

UCLASS()
class TAT_API UTATLocalPlayerAnalyticsSubsystem : public ULocalPlayerSubsystem
{
   GENERATED_BODY()
public:
   virtual void Initialize(FSubsystemCollectionBase& collection) override;

   void HandleMatchStart() const;
   void HandleMatchEnd(const FMatchPersistentData& data) const;
private:
   void _SetPlayerDetails(FTATAnalyticsCustomFields& analyticsFields) const;
   void _SetMatchDetails(FTATAnalyticsCustomFields& analyticsFields) const;
   
   double _TimeBooted { 0 };
};
