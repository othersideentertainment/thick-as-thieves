// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Variation/Clues/TATClueSpawner.h"

// tat
#include "Variation/Clues/TATClueSpawnerSubsystem.h"

// ue
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATClueSpawner)


UTATClueSpawnerComponent::UTATClueSpawnerComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   bAutoActivate = true;
}

void UTATClueSpawnerComponent::BeginPlay()
{
   Super::BeginPlay();

   if(UTATClueSpawnerSubsystem* subsystem = GetWorld()->GetSubsystem<UTATClueSpawnerSubsystem>())
   {
      subsystem->RegisterSpawner(this);
   }
}

void UTATClueSpawnerComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   // Don't bother if tearing down the level
   if(endPlayReason == EEndPlayReason::Destroyed || endPlayReason == EEndPlayReason::RemovedFromWorld)
   {
      if(UTATClueSpawnerSubsystem* subsystem = GetWorld()->GetSubsystem<UTATClueSpawnerSubsystem>())
      {
         subsystem->UnregisterSpawner(this);
      }
   }
   
   Super::EndPlay(endPlayReason);
}

#if WITH_EDITOR
void UTATClueSpawnerComponent::CheckForErrors()
{
   Super::CheckForErrors();
   
   FMessageLog messageLog("MapCheck");
   auto makeToken = [this] { return FUObjectToken::Create(GetOwner(), FText::FromString(GetReadableName())); };
   _sceneRequirement.ValidateRequirement(messageLog, GetOwner(), makeToken);
}
#endif

FTATClueBucketKey UTATClueSpawnerComponent::GetClueBucket() const
{
   return {_clueType};
}

