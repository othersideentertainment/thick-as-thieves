// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Quests/Spawn/TATQuestActorSpawner.h"

// tat
#include "Developer/TATProjectSettings.h"
#include "Quests/Spawn/TATQuestActorSpawnAction.h"
#include "Quests/TATActiveQuestSubsystem.h"

// ue
#include "Engine/AssetManager.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "StructUtils/InstancedStruct.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATQuestActorSpawner)


UTATQuestActorSpawnerComponent::UTATQuestActorSpawnerComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   bWantsInitializeComponent = true;
}

void UTATQuestActorSpawnerComponent::ExecuteSpawn(const TSoftClassPtr<AActor>& actorClass)
{
   UAssetManager::GetStreamableManager().RequestAsyncLoad(actorClass.ToSoftObjectPath(), [actorClass, weakThis = MakeWeakObjectPtr(this)]
      {
         if(UTATQuestActorSpawnerComponent* self = weakThis.Get())
         {
            const FTransform& spawnXfm = self->GetComponentTransform();
            FActorSpawnParameters spawnParams;
            spawnParams.bNoFail = true;
            AActor* spawnedActor = self->GetWorld()->SpawnActor(actorClass.Get(), &spawnXfm, spawnParams);

            if (spawnedActor)
            {
               self->OnActorSpawned.Broadcast(spawnedActor);

               for(const FInstancedStruct& spawnAction : self->_spawnActions)
               {
                  if(const FTATQuestActorSpawnAction* action = spawnAction.GetPtr<FTATQuestActorSpawnAction>())
                  {
                     action->OnSpawnedQuestActor(spawnedActor, self);
                  }
               }
            }
         }
      });
}

#if WITH_EDITOR
void UTATQuestActorSpawnerComponent::CheckForErrors()
{
   FMessageLog messageLog("MapCheck");
   if(IsEnabled())
   {
      auto makeToken = [this] { return FUObjectToken::Create(GetOwner(), FText::FromString(GetReadableName())); };
      _sceneRequirement.ValidateRequirement(messageLog, GetOwner(), makeToken);
   
      if (!_questLocationTag.IsValid())
      {
         messageLog.Warning()
            ->AddToken(makeToken())
            ->AddToken(FTextToken::Create(FText::FromString(TEXT("No quest location tag on quest spawner"))));
      }

      if(UTATProjectSettings::Get().RequireClueLocationTags)
      {
         if(!_clueLocationTag.IsValid())
         {
            messageLog.Error()
                  ->AddToken(makeToken())
                  ->AddToken(FTextToken::Create(INVTEXT("Quest spawner does not have clue location tag")));
         }

         if(_clueLocationText.IsEmpty())
         {
            messageLog.Error()
                  ->AddToken(makeToken())
                  ->AddToken(FTextToken::Create(INVTEXT("Quest spawner does not have clue location text")));
         }
      }

      if (_clueLocationText.IsEmpty())
      {
         messageLog.Warning()
            ->AddToken(makeToken())
            ->AddToken(FTextToken::Create(FText::FromString(TEXT("No location text for quest spawner"))));
      }
      for(const FInstancedStruct& spawnAction : _spawnActions)
      {
         if(const FTATQuestActorSpawnAction* action = spawnAction.GetPtr<FTATQuestActorSpawnAction>())
         {
            action->Validate([&](const FText& message)
            {
               messageLog.Error()
                  ->AddToken(makeToken())
                  ->AddToken(FTextToken::Create(INVTEXT("Action ")))
                  ->AddToken(FTextToken::Create(message));
            });
         }
      }
   }
}
#endif

void UTATQuestActorSpawnerComponent::InitializeComponent()
{
   Super::InitializeComponent();

   if (UTATActiveQuestSubsystem* questSubsystem = GetWorld()->GetSubsystem<UTATActiveQuestSubsystem>())
   {
      questSubsystem->RegisterActorSpawner(this);
   }
}

ATATQuestActorSpawner::ATATQuestActorSpawner(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer.DoNotCreateDefaultSubobject("Root"))
{
   PrimaryActorTick.bCanEverTick = false;
   bNetLoadOnClient = false; // this is a server-only object that spawns in the actual thing we care about

   // Doing this rather than overriding the default subobject for the root so the name remains "Spawner"
   _questSpawner = CreateDefaultSubobject<UTATQuestActorSpawnerComponent>(TEXT("Spawner"));
   _questSpawner->SetMobility(EComponentMobility::Static);
   _SetRootComponent(_questSpawner);
}
