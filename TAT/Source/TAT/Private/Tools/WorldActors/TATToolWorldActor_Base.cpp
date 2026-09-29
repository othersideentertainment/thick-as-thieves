// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/WorldActors/TATToolWorldActor_Base.h"

// tat
#include "AI/Perception/TATAISense_Sight.h"
#include "AI/Perception/TATPerceptionFunctionLibrary.h"
#include "Character/TATTeams.h"
#include "Developer/TATToolSettings.h"
#include "Items/TATItemFunctionLibrary.h"
#include "Tools/TATToolComponent.h"
#include "Tools/TATToolSetComponent.h"

// ose
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"

// ue5
#include "GameFramework/Character.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATToolWorldActor_Base)


ATATToolWorldActor_Base::ATATToolWorldActor_Base()
{
   PrimaryActorTick.bCanEverTick = false;
   PrimaryActorTick.bStartWithTickEnabled = false;
   
   _perceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSourceComponent"));

   bReplicates = true;
}

void ATATToolWorldActor_Base::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(ATATToolWorldActor_Base, _parentToolClass, COND_InitialOnly);
}

void ATATToolWorldActor_Base::BeginPlay()
{
   Super::BeginPlay();
   
   if (ensure(_perceptionStimuliSource) && _shouldRegisterForPerceptionSource)
   {
      _perceptionStimuliSource->RegisterForSense(UTATAISense_Sight::StaticClass());
   }
}

void ATATToolWorldActor_Base::AuthorityDeploy_Implementation(const FTATGearWorldActorParameters& worldActorParams)
{
   _parentToolClass = worldActorParams.ParentToolClass;
}

bool ATATToolWorldActor_Base::CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation,
   int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor, const bool* wasVisible,
   int32* userData) const
{
   static constexpr bool kNonColliding = false;
   static constexpr bool kIncludeFromChildActors = false;
   const FBox actorBounds = GetComponentsBoundingBox(kNonColliding, kIncludeFromChildActors);
   return UTATPerceptionFunctionLibrary::CanActorBoundingBoxBeSeenFromLocation(
      this,
      actorBounds,
      observerLocation,
      outSeenLocation,
      numberOfLoSChecksPerformed,
      outSightStrength,
      ignoreActor
   );
}

uint8 ATATToolWorldActor_Base::GetTeam() const
{
   if(const IOSETeamInterface* ownerTeamInterface = Cast<IOSETeamInterface>(GetOwner()))
   {
      return UTATTeamAttitudeSolver::GetOriginalTeam(ownerTeamInterface->GetTeam());
   }
   return IOSETeamInterface::GetTeam();
}

int32 ATATToolWorldActor_Base::GetAmmoCostByUsageType(FGameplayTag ToolUsageTag) const
{
   if (!_parentToolClass)
   {
      return 0;
   }

   if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(GetInstigator()))
   {
      if (UTATToolSetComponent* tatToolset = Cast<UTATToolSetComponent>(toolSetInterface.GetObject()))
      {
         return tatToolset->GetAmmoCostByUsageType(GetParentToolClass(), ToolUsageTag);
      }
   }

   return 0;
}

bool ATATToolWorldActor_Base::IsInteractable_Implementation(ACharacter* interactingCharacter) const
{
   // If we don't have a parent tool, then we can't be picked up,
   // since we won't know which tool to give to the interacting player
   if (!_parentToolClass)
   {
      return false;
   }

   // Can only be interacted with by a character who can pick up tools/ammo
   if (!interactingCharacter->Implements<UToolSetSystemInterface>() || !UTATItemFunctionLibrary::CanCharacterPickUpThings(interactingCharacter))
   {
      return false;
   }

   if (DeployablePickupCapability == ETATDeployablePickupCapability::Never)
   {
      return false;
   }
   else if (DeployablePickupCapability == ETATDeployablePickupCapability::BeforeActivation)
   {
      return !_HasBeenActivated();
   }
   else if (DeployablePickupCapability == ETATDeployablePickupCapability::Always)
   {
      return true;
   }
   else
   {
      checkf(false, TEXT("Unexpected value for DeployablePickupCapability: %d"), static_cast<int32>(DeployablePickupCapability));
      return false;
   }
}

void ATATToolWorldActor_Base::GetInteractPrompt_Implementation(ACharacter* interactingCharacter, FInteractPrompt& prompt)
{
   check(_parentToolClass);
   if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(interactingCharacter))
   {
      if (UTATToolSetComponent* tatToolset = Cast<UTATToolSetComponent>(toolSetInterface.GetObject()))
      {
         const int32 ammoCount = _GetAmmoCountToAdd();
         if (tatToolset->HasRoomToAddToolWithAmmo(_parentToolClass, ammoCount))
         {
            prompt.HoldAction = UTATToolSettings::Get().PickupDeployablePrompt;
         }
         else if (AllowPlayerToDestroyIfAmmoFull)
         {
            prompt.HoldAction = UTATToolSettings::Get().DestroyDeployablePrompt;
         }
         else
         {
            prompt.ErrorMessage = UTATToolSettings::Get().CannotPickupDeployableDueToAmmoLimitPrompt;
         }
      }
   }
}

FInteractStartResult ATATToolWorldActor_Base::StartInteract_Implementation(ACharacter* interactingCharacter)
{
   FInteractStartResult startResult;
   if (!ensure(_parentToolClass))
   {
      return startResult;
   }
   if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(interactingCharacter))
   {
      if (UTATToolSetComponent* tatToolset = Cast<UTATToolSetComponent>(toolSetInterface.GetObject()))
      {
         const int32 ammoCount = _GetAmmoCountToAdd();
         if (tatToolset->HasRoomToAddToolWithAmmo(_parentToolClass, ammoCount) || AllowPlayerToDestroyIfAmmoFull)
         {
            startResult = FInteractStartResult::Wait(UTATToolSettings::Get().PickupDeployableHoldDuration);
            startResult.HoldAnimationTag = UTATToolSettings::Get().PickupDeployableHoldAnimationTag;
            startResult.HoldActionCues = PickUpHoldActionCues;
         }
      }
   }

   return startResult;
}

bool ATATToolWorldActor_Base::EndInteract_Implementation(ACharacter* interactingCharacter, const FInteractEndContext& context)
{
   if (context.IsComplete())
   {
      if (HasAuthority())
      {
         if (!ensure(_parentToolClass))
         {
            return false;
         }

         if (TScriptInterface<IToolSetInterface> toolSetInterface = IToolSetInterface::GetToolSetFromActor(interactingCharacter))
         {
            if (UTATToolSetComponent* tatToolset = Cast<UTATToolSetComponent>(toolSetInterface.GetObject()))
            {
               const int32 ammoCount = _GetAmmoCountToAdd();
               bool hadRoomForAmmo = tatToolset->AuthorityTryAddToolWithAmmo(_parentToolClass, ammoCount);
               if (hadRoomForAmmo || AllowPlayerToDestroyIfAmmoFull)
               {
                  Destroy();
               }
            }
         }
      }
   }

   return true;
}

int32 ATATToolWorldActor_Base::_GetAmmoCountToAdd() const
{
   // For now, one deployable is always one ammo
   // In the future, we may want this to be configurable via the actor or the tool
   return 1;
}
