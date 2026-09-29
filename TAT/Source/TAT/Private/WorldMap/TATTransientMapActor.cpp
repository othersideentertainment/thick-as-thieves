// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "WorldMap/TATTransientMapActor.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "WorldMap/TATMapActorComponent.h"
#include "WorldMap/TATTransientMapActorDataAsset.h"

// ue
#include "Engine/AssetManager.h"
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTransientMapActor)
DEFINE_LOG_CATEGORY_STATIC(LogTATTransientMapActor, Log, All);

ATATTransientMapActor::ATATTransientMapActor()
{
   MapActorComponent = CreateDefaultSubobject<UTATMapActorComponent>(TEXT("MapActorComponent"));
   MapActorComponent->bAutoActivate = false;

   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
   bReplicates = true;
}

void ATATTransientMapActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(ATATTransientMapActor, TransientMapActorIdentifier, COND_InitialOnly);
   DOREPLIFETIME_CONDITION(ATATTransientMapActor, MapSpriteOverride, COND_InitialOnly);
}

void ATATTransientMapActor::BeginPlay()
{
   Super::BeginPlay();

   if (IsNetMode(NM_DedicatedServer))
   {
      return;
   }

   // No-op for instances spawned on listen server for remote clients
   if (IsNetMode(NM_ListenServer) && bOnlyRelevantToOwner)
   {
      if (APlayerController* owner = Cast<APlayerController>(GetOwner()))
      {
         if (!owner->IsLocalController())
         {
            return;
         }
      }
   }

   if (!TransientMapActorIdentifier.IsValid())
   { 
      UE_LOG(LogTATTransientMapActor, Error, TEXT("Spawned with invalid TransientMapActorIdentifier!"));
      return;
   }
   const TSoftObjectPtr<UTATTransientMapActorDataAsset> transientMapActorDataAssetSoftPtr = UTATProjectSettings::Get().TransientMapActorData;
   if (transientMapActorDataAssetSoftPtr.IsNull())
   {
      UE_LOG(LogTATTransientMapActor, Error, TEXT("Invalid TransientMapActorData in TATProjectSettings!"));
      return;
   }

   TWeakObjectPtr<ATATTransientMapActor> weakThis(this);
   UAssetManager::GetStreamableManager().RequestAsyncLoad(transientMapActorDataAssetSoftPtr.ToSoftObjectPath(), [weakThis, transientMapActorDataAssetSoftPtr]
      {
         if (ATATTransientMapActor* transientMapActor = weakThis.Get())
         {
            const UTATTransientMapActorDataAsset* transientMapActorDataAsset = transientMapActorDataAssetSoftPtr.Get();
            if (const FTATMapRepresentationData* mapRepresentationDataPtr = transientMapActorDataAsset->GetMapRepresentationData(transientMapActor->TransientMapActorIdentifier))
            {
               // Apply sprite override if provided
               FTATMapRepresentationData mapRepresentationData = *mapRepresentationDataPtr;
               if (transientMapActor->MapSpriteOverride.IsValid())
               {
                  mapRepresentationData.DefaultSpriteTag = transientMapActor->MapSpriteOverride;
               }
               transientMapActor->MapActorComponent->SetMapRepresentationData(mapRepresentationData);
               transientMapActor->MapActorComponent->Activate();
            }
         }
      });
}
