// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "OSESaveable.generated.h"



USTRUCT()
struct FOSEActorSaveDataRecord
{
   GENERATED_BODY()
public:

   UPROPERTY(SaveGame)
      UClass* actorClass = nullptr;

   UPROPERTY(SaveGame)
      FTransform actorTransform;

   UPROPERTY(SaveGame)
      FString actorName;

   //extra data for actor specific implementations
   UPROPERTY(SaveGame)
      TArray<uint8> data;
};

USTRUCT()
struct FOSEActorSaveDataRecordArray
{
   GENERATED_BODY()
public:

   UPROPERTY(SaveGame)
   TArray< FOSEActorSaveDataRecord> entries;
};

UINTERFACE(MinimalAPI, meta = (CannotImplementInterfaceInBlueprint))
class UOSESaveable : public UInterface
{
   GENERATED_UINTERFACE_BODY()
public:
};

class IOSESaveable
{
   GENERATED_IINTERFACE_BODY()

public:

   virtual void SaveToRecord(FOSEActorSaveDataRecord& record) = 0;
   virtual void LoadFromRecord(const FOSEActorSaveDataRecord Record)  = 0;
};
