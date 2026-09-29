// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "WorldMap/TATWorldMapTypes.h"

// ue
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "TATTransientMapActorDataAsset.generated.h"

USTRUCT()
struct TAT_API FTATTransientMapActorEntry
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly, meta = (Categories = "TransientMapActor"))
   FGameplayTag Identifier;

   UPROPERTY(EditDefaultsOnly)
   FTATMapRepresentationData MapRepresentationData;

   FORCEINLINE bool operator==(FGameplayTag identifier) const { return identifier == Identifier; }
};

UCLASS()
class TAT_API UTATTransientMapActorDataAsset : public UDataAsset
{
   GENERATED_BODY()
   

#if WITH_EDITOR
   // From UObject
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif // WITH_EDITOR

public:
   const FTATMapRepresentationData* GetMapRepresentationData(FGameplayTag identifier) const;

   UPROPERTY(EditDefaultsOnly)
   TArray<FTATTransientMapActorEntry> TransientMapActorEntries;
};
