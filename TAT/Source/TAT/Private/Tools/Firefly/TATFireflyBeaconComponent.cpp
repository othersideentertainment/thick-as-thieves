// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Tools/Firefly/TATFireflyBeaconComponent.h"

// tat
#include "Tools/Firefly/TATFireflySubsystem.h"

// ue
#include "GameplayTagAssetInterface.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATFireflyBeaconComponent)

UTATFireflyBeaconComponent::UTATFireflyBeaconComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTATFireflyBeaconComponent::SetShown(bool isShown)
{
   if(isShown != _shown)
   {
      _shown = isShown;
      _OnShownChanged(_shown);
   }
}

bool UTATFireflyBeaconComponent::IsAllowed() const
{
   if (!_allowed)
   {
      return false;
   }

   if (_requiredTags.IsValid() || _blockedTags.IsValid())
   {
      if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(GetOwner()))
      {
         if (!tagInterface->HasAllMatchingGameplayTags(_requiredTags))
         {
            return false;
         }
         if (tagInterface->HasAnyMatchingGameplayTags(_blockedTags))
         {
            return false;
         }
      }
   }

   return true;
}

void UTATFireflyBeaconComponent::BeginPlay()
{
   Super::BeginPlay();

   if(UTATFireflySubsystem* fireflySubsystem = GetWorld()->GetSubsystem<UTATFireflySubsystem>())
   {
      fireflySubsystem->RegisterBeacon(this);
   }
   
}

void UTATFireflyBeaconComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   Super::EndPlay(endPlayReason);

   if(endPlayReason == EEndPlayReason::Destroyed || endPlayReason == EEndPlayReason::RemovedFromWorld)
   {
      if(UTATFireflySubsystem* fireflySubsystem = GetWorld()->GetSubsystem<UTATFireflySubsystem>())
      {
         fireflySubsystem->UnregisterBeacon(this);
      }
   }
}

void UTATFireflyBeaconComponent::_OnShownChanged_Implementation(bool shown)
{
}


