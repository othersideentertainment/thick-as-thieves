// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"

#include "TATSignificanceBasedTick.generated.h"


USTRUCT()
struct FTATSigificanceTickLodThreshold
{
   GENERATED_BODY()
   
   UPROPERTY(EditDefaultsOnly, meta=(Units="Cm"))
   float MaxDistanceToViewer = 0;
   UPROPERTY(EditDefaultsOnly)
   float TickInterval = 0;
};

UCLASS()
class TAT_API UTATSignificanceBasedTickConfig : public UDataAsset
{
   GENERATED_BODY()

public:
   UPROPERTY(EditDefaultsOnly, meta=(TitleProperty="<{MaxDistanceToViewer}cm => {TickInterval}"))
   TArray<FTATSigificanceTickLodThreshold> SignificanceThresholds;

   static void RegisterComponent(UActorComponent* component, const UTATSignificanceBasedTickConfig* config);
   static void RegisterActor(AActor* actor, const UTATSignificanceBasedTickConfig* config);
   static void Unregister(UObject* object);
};
