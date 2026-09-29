// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "TATShockFloorUtilities.generated.h"




USTRUCT(BlueprintType)
struct FTATShockFloorPlaneEntry
{
   GENERATED_BODY()

   UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (MakeEditWidget))
   FVector Location = FVector(ForceInitToZero);

   UPROPERTY(EditAnywhere, BlueprintReadOnly)
   FVector2D DesiredSize = FVector2D(200, 200);
};

UCLASS()
class TAT_API UTATShockFloorUtilities : public UObject
{
	GENERATED_BODY()
	
};
