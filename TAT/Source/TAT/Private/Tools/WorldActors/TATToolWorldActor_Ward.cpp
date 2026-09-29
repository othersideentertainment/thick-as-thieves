// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/WorldActors/TATToolWorldActor_Ward.h"

// tat
#include "Tools/TATWardToolComponent.h"
#include "Breakables/TATBreakableActorImpl.h"

// ose
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"

// ue5
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"

#include "DrawDebugHelpers.h"
#include "GameFramework/GameStateBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolWorldActor_Ward)

DEFINE_LOG_CATEGORY_STATIC(LogTATToolWorldActor_Ward, Log, All)

BREAKABLE_ACTOR_IMPLS(ATATToolWorldActor_Ward, _abilitySystemComponent, _breakableComponent)

ATATToolWorldActor_Ward::ATATToolWorldActor_Ward()
{
   _abilitySystemComponent = CreateAbilitySystemForBreakables(this);
   _breakableComponent = CreateDefaultSubobject<UTATBreakableComponent>(TEXT("BreakableComponent"));
   bReplicateUsingRegisteredSubObjectList = true;

   NetDormancy = DORM_Initial;
}

void ATATToolWorldActor_Ward::BeginPlay()
{
   Super::BeginPlay();

   // Notify the warded actor that the ward is activated
   if (WardedActor != nullptr)
   {
      if (WardedActor->Implements<UTATWardableInterface>())
      {
         ITATWardableInterface::Execute_OnWardActivated(WardedActor, this, GetInstigator());
      }
      else
      {
         UE_LOG(LogTATToolWorldActor_Ward, Error, TEXT("WardedActor '%s' does not implement TATWardableInterface"), *WardedActor->GetActorNameOrLabel());
      }
   }

   if (HasAuthority())
   {
      if (UTATWardToolComponent* wardTool = _GetWardTool())
      {
         wardTool->AuthorityAddSpawnedWard(this);
      }
   }
}

void ATATToolWorldActor_Ward::EndPlay(EEndPlayReason::Type reason)
{
   // Notify the warded actor that the ward is deactivated
   if (WardedActor != nullptr && WardedActor->Implements<UTATWardableInterface>())
   {
      ITATWardableInterface::Execute_OnWardDeactivated(WardedActor, this);
   }

   if (HasAuthority())
   {
      if (UTATWardToolComponent* wardTool = _GetWardTool())
      {
         wardTool->AuthorityRemoveSpawnedWard(this);
      }
   }

   Super::EndPlay(reason);
}

void ATATToolWorldActor_Ward::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(ATATToolWorldActor_Ward, WardedActor, COND_InitialOnly);
   DOREPLIFETIME_CONDITION(ATATToolWorldActor_Ward, WardBoxExtent, COND_InitialOnly);
   DOREPLIFETIME_CONDITION(ATATToolWorldActor_Ward, OwningPlayerState, COND_InitialOnly);
   DOREPLIFETIME_CONDITION(ATATToolWorldActor_Ward, ServerSpawnTimeSeconds, COND_InitialOnly);
}

bool ATATToolWorldActor_Ward::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   if (OwningPlayerState && interactingCharacter && interactingCharacter->GetPlayerState() != OwningPlayerState)
   {
      return false;
   }
   return Super::IsInteractable_Implementation(interactingCharacter);
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

void ATATToolWorldActor_Ward::AuthoritySetupWardBeforeFinishSpawning(AActor* wardedActor, const FVector& wardBoxExtent, APlayerState* owningPlayerState)
{
   check(HasAuthority());
#if !UE_BUILD_SHIPPING
   ensure(UE::Net::FTearOffSetter::IsActorInDeferredConstruction(this));
#endif
   WardedActor = wardedActor;
   WardBoxExtent = wardBoxExtent;
   OwningPlayerState = owningPlayerState;
   ServerSpawnTimeSeconds = GetWorld()->GetTimeSeconds();
}

float ATATToolWorldActor_Ward::GetServerLifeSpanRemaining() const
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

UTATWardToolComponent* ATATToolWorldActor_Ward::_GetWardTool() const
{
   TSubclassOf<UTATToolComponent> parentToolType = GetParentToolClass();
   if (!parentToolType || !parentToolType->IsChildOf(UTATWardToolComponent::StaticClass()))
   {
      return nullptr;
   }

   // We're expecting a child class, not the native base class
   ensure(parentToolType != UTATWardToolComponent::StaticClass());

   if (IToolSetSystemInterface* toolSetSystem = Cast<IToolSetSystemInterface>(GetInstigator()))
   {
      if (TScriptInterface<IToolSetInterface> toolSetInterface = toolSetSystem->GetToolSetInterface())
      {
         constexpr bool matchesExact = true;
         return Cast<UTATWardToolComponent>(toolSetInterface->GetToolByClass(parentToolType, matchesExact));
      }
   }

   return nullptr;
}
