// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATInhibitorActor.h"

// tat
#include "Environment/TATInhibitorSubsystem.h"

// ue
#include "Net/UnrealNetwork.h"
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInhibitorActor)

DEFINE_LOG_CATEGORY_STATIC(LogTATInhibitorActor, Log, All)

ATATInhibitorActor::ATATInhibitorActor()
{
   bReplicates = true;
   NetDormancy = DORM_Initial;
}

void ATATInhibitorActor::BeginPlay()
{
   Super::BeginPlay();

   // Notify the actor that an inhibitor was activated
   if (IsValid(InhibitedActor))
   {
      if (InhibitedActor->Implements<UTATInhibitableInterface>())
      {
         int32 newInhibitorCount = 1;
         if (UTATInhibitorSubsystem* inhibitorSubsystem = GetWorld()->GetSubsystem<UTATInhibitorSubsystem>())
         {
            inhibitorSubsystem->NotifyActorApplyInhibitor(InhibitedActor, this, newInhibitorCount);
         }
         ITATInhibitableInterface::Execute_OnInhibitorActivated(InhibitedActor, this, GetInstigator(), newInhibitorCount);
      }
      else
      {
         UE_LOG(LogTATInhibitorActor, Error, TEXT("InhibitedActor '%s' does not implement TATInhibitableInterface"), *InhibitedActor->GetActorNameOrLabel());
      }
   }
}

void ATATInhibitorActor::EndPlay(EEndPlayReason::Type reason)
{
   // Notify the actor that the inhibitor was deactivated
   if (IsValid(InhibitedActor) && InhibitedActor->Implements<UTATInhibitableInterface>())
   {
      int32 newInhibitorCount = 0;
      if (UTATInhibitorSubsystem* inhibitorSubsystem = GetWorld()->GetSubsystem<UTATInhibitorSubsystem>())
      {
         inhibitorSubsystem->NotifyActorRemoveInhibitor(InhibitedActor, this, newInhibitorCount);
      }
      const bool allInhibitorsRemoved = newInhibitorCount <= 0;
      ITATInhibitableInterface::Execute_OnInhibitorDeactivated(InhibitedActor, this, newInhibitorCount, allInhibitorsRemoved);
   }

   Super::EndPlay(reason);
}

void ATATInhibitorActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(ATATInhibitorActor, InhibitedActor, COND_InitialOnly);
   DOREPLIFETIME_CONDITION(ATATInhibitorActor, PlacementInfo, COND_InitialOnly);
   DOREPLIFETIME_CONDITION(ATATInhibitorActor, InhibitorColor, COND_InitialOnly);
   DOREPLIFETIME_CONDITION(ATATInhibitorActor, OwningPlayerState, COND_InitialOnly);
   DOREPLIFETIME_CONDITION(ATATInhibitorActor, ServerSpawnTimeSeconds, COND_InitialOnly);
}

#if !UE_BUILD_SHIPPING
namespace UE::Net
{
   /// Hack to check the (normally private) bHasFinishedSpawning flag in AActor.
   /// Only used as an ensure in non-shipping builds, so this is mostly harmless and safe to remove if this hack ever breaks.
   class FTearOffSetter
   {
   public:
      static bool IsActorInDeferredConstruction(AActor* actor) { return actor != nullptr && !actor->bHasFinishedSpawning; }
   };
}
#endif

void ATATInhibitorActor::AuthoritySetupBeforeFinishSpawning(AActor* inhibitedActor, const FTATInhibitorPlacementInfo& placementInfo, const FLinearColor& color, APlayerState* owningPlayerState)
{
   check(HasAuthority());
#if !UE_BUILD_SHIPPING
   ensure(UE::Net::FTearOffSetter::IsActorInDeferredConstruction(this));
#endif
   InhibitedActor = inhibitedActor;
   PlacementInfo = placementInfo;
   InhibitorColor = color;
   OwningPlayerState = owningPlayerState;
   ServerSpawnTimeSeconds = GetWorld()->GetTimeSeconds();
}

float ATATInhibitorActor::GetInhibitorLifeSpanRemaining() const
{
   if (InitialLifeSpan > 0 && ServerSpawnTimeSeconds != 0)
   {
      if (AGameStateBase* gameState = GetWorld()->GetGameState())
      {
         const float secondsSinceServerSpawn = gameState->GetServerWorldTimeSeconds() - ServerSpawnTimeSeconds;
         return FMath::Clamp(InitialLifeSpan - secondsSinceServerSpawn, 0.0f, InitialLifeSpan);
      }
   }
   return 0.0f;
}
