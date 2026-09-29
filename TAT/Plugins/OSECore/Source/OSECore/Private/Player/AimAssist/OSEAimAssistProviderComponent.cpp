// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Player/AimAssist/OSEAimAssistProviderComponent.h"

// ose
#include "Player/OSEPlayerController.h"
#include "Player/AimAssist/OSEAimAssistComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEAimAssistProviderComponent)

// ue4

UOSEAimAssistProviderComponent::UOSEAimAssistProviderComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = true;
   PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
}

void UOSEAimAssistProviderComponent::BeginPlay()
{
   Super::BeginPlay();
   SetComponentTickEnabled(Enabled);
   _UpdateAimAssistTarget();
}

void UOSEAimAssistProviderComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   if (Enabled)
   {
      QUICK_SCOPE_CYCLE_COUNTER(STAT_OSEAimAssistProviderComponent_Tick);
      // TODO: Could try and cache this off if it's costing too much in per-frame lookups.  Can't just cache it off in
      // BeginPlay() because it often won't exist as a client spawning in
      AOSEPlayerController* pc = AOSEPlayerController::GetLocalOSEPlayerController(this);
      APawn* pawn = pc ? pc->GetPawn() : nullptr;
      UOSEAimAssistComponent* aimAssist = pc ? pc->GetAimAssist() : nullptr;

      if (pc && pawn && aimAssist)
      {
         // Filtering on this side includes early-out decisions like distance
         // and maybe in the future we'll add some contextual filtering here -- maybe this component
         // only wants to be considered when we're in combat, or have a weapon out, or something similar.
         // It'd be trivial to subclass this game-side to add in many game-specific considerations.

         if (_ShouldAddTarget(FAimAssistProviderContext(GetOwner(), pawn)))
         {
            aimAssist->AddAimAssistTarget(_aimAssistTarget);
         }
      }
   }
}

void UOSEAimAssistProviderComponent::SetAimAssistBoundsComponent(USceneComponent* component)
{
   _boundsComponent = component;
   _UpdateAimAssistTarget();
}

void UOSEAimAssistProviderComponent::SetAimAssistCenterComponent(USceneComponent* component)
{
   _centerComponent = component;
   _UpdateAimAssistTarget();
}

bool UOSEAimAssistProviderComponent::_ShouldAddTarget(const FAimAssistProviderContext& context) const
{
   check(context.PlayerPawn);
   const FVector ownLocation = _centerComponent ? _centerComponent->GetComponentLocation() : GetOwner()->GetActorLocation();
   if (FVector::DistSquared(context.PlayerPawn->GetActorLocation(), ownLocation) >= FMath::Square(MaxAimAssistDistance))
   {
      return false;
   }

   for (TSubclassOf<UOSEAimAssistProviderPredicate> condition : AimAssistConditions)
   {
      if (condition && !condition.GetDefaultObject()->ShouldAddTarget(context))
      {
         return false;
      }
   }

   return true;
}

void UOSEAimAssistProviderComponent::_UpdateAimAssistTarget()
{
   _aimAssistTarget.InnerBoxSizeMultiplier = InnerBoxSizeMultiplier;
   _aimAssistTarget.OuterBoxSizeMultiplier = OuterBoxSizeMultiplier;
   _aimAssistTarget.CenterComponent = _centerComponent;
   _aimAssistTarget.CenterOffset = CenterComponentOffset;
   

   // priority for bounds is the _boundsComponent, then actor.
   // don't use "this" for our bounds because this component doesn't have a reasonable bounding volume
   _aimAssistTarget.BoundsComponent = _boundsComponent != nullptr ? _boundsComponent : nullptr;
   _aimAssistTarget.BoundsActor = !_aimAssistTarget.BoundsComponent.IsValid() ? GetOwner() : nullptr;
}

