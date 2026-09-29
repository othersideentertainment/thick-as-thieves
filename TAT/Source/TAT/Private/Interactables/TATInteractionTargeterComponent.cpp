// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATInteractionTargeterComponent.h"

// tat
#include "Interactables/TATSupportInteractionByInterface.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Interactables/InteractableInterface.h"
#include "Interactables/OSEInteractionHelpers.h"

// ue4
#include "GameFramework/Controller.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "DrawDebugHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATInteractionTargeterComponent)

namespace InteractCVars
{
   static int32 DrawRange = 0;
   FAutoConsoleVariableRef CVarEnableClearanceCheck(
      TEXT("TAT.Interact.DrawRange"),
      DrawRange,
      TEXT("Enables debug drawing of the interact range"),
      ECVF_Default);
}

UTATInteractionTargeterComponent::UTATInteractionTargeterComponent()
{
   // Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
   // off to improve performance if you don't need them.
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bAllowTickOnDedicatedServer = false;
}

void UTATInteractionTargeterComponent::BeginPlay()
{
   Super::BeginPlay();

   APawn* pawn = Cast<APawn>(GetOwner());
   check(pawn);

   // TODO: revisit this, this replicates the old blueprint logic, but we may not want the local player to be authoritative here
   // NOTE: The controller might not be assigned to the pawn when late-spawned in a non-dedicated server, so have to check every tick for now
   PrimaryComponentTick.SetTickFunctionEnable(pawn != nullptr);
}

void UTATInteractionTargeterComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   if(IsTargetingFrozen())
   {
      return;
   }
   
   ACharacter* ownerCharacter = CastChecked<ACharacter>(GetOwner());
   AController* controller = ownerCharacter->GetController();

   // A player might not have a controller at the moment (e.g., piloting drone)
   if (!controller || _targetingSuppressed)
   {
      // Act as if we had cast, and hit nothing.
      if (_targetInteractable.GetObject())
      {
         SetNewTarget(nullptr);
         RefreshPrompt(ownerCharacter);
      }
      return;
   }

   // if this is the server, leave it alone
   if(!controller->IsLocalController())
   {
      return;
   }

   TScriptInterface<IInteractableInterface> found = _FindNewTarget(ownerCharacter);
   if (found != _targetInteractable)
   {
      SetNewTarget(found);
   }

   // brute force the prompt, since it could change at any time
   RefreshPrompt(ownerCharacter);
}

TScriptInterface<IInteractableInterface> UTATInteractionTargeterComponent::GetCurrentTarget() const
{
   return _targetInteractable;
}

void UTATInteractionTargeterComponent::SetNewTarget(TScriptInterface<IInteractableInterface> newTarget)
{
   if(_targetInteractable.GetObject())
   {
      IInteractableInterface::Execute_ShowHighlight(_targetInteractable.GetObject(), false);
   }

   _targetInteractable = newTarget;

   if(_targetInteractable.GetObject())
   {
      IInteractableInterface::Execute_ShowHighlight(_targetInteractable.GetObject(), true);
   }

   OnTargetChanged.Broadcast(_targetInteractable);
}

void UTATInteractionTargeterComponent::RefreshPrompt(ACharacter* ownerCharacter)
{
   FInteractPrompt prompt;
   if (_targetInteractable.GetObject())
   {
      IInteractableInterface::Execute_GetInteractPrompt(_targetInteractable.GetObject(), ownerCharacter, prompt);
   }

   if (!prompt.IdenticalTo(_currentPrompt))
   {
      _currentPrompt = prompt;
      OnPromptChanged.Broadcast(_currentPrompt);
   }
}

float UTATInteractionTargeterComponent::_GetCurrentInteractRange() const
{
   if (const APawn* pawn = Cast<APawn>(GetOwner()))
   {
      // increase range when view is pitched downwards so that it can reach the same horizontal distance
      // as when straight on. This should make picking up items off the ground easier without having to
      // increase the interaction range in general.
      const FRotator viewRotation = pawn->GetViewRotation();
      const float downLength = UE_INV_SQRT_2 * 2.f * MaxInteractionRange; // Divide by Cos(45 degrees) to get distance for same horizontal reach
      return FMath::GetMappedRangeValueClamped(FVector2D(0, 45.f), FVector2D(downLength, MaxInteractionRange), FMath::FindDeltaAngleDegrees(-45.f, viewRotation.Pitch));
   }
   return MaxInteractionRange;
}

TScriptInterface<IInteractableInterface> UTATInteractionTargeterComponent::_FindNewTarget(ACharacter* ownerCharacter)
{
   if (_strategyOverride != nullptr)
   {
      return IInteractionTargetStrategy::Execute_FindTargetInteractable(_strategyOverride, ownerCharacter);
   }

   FVector startPos;
   FVector endPos;
   if (UOSEAbilityFunctionLibrary::OffsetCameraAimToPhysicalAim(GetOwner(), FGameplayAbilityTargetingLocationInfo(), _GetCurrentInteractRange(), startPos, endPos))
   {
#if ENABLE_DRAW_DEBUG
      if (InteractCVars::DrawRange)
      {
         DrawDebugLine(GetWorld(), startPos, endPos, FColor::Blue, false);
         DrawDebugSphere(GetWorld(), endPos, 10, 10, FColor::Blue);
      }
#endif // ENABLE_DRAW_DEBUG
      {
         TScriptInterface<IInteractableInterface> found = UOSEInteractionHelpers::CastForInteractables(ownerCharacter, startPos, endPos, _GetTraceResponseParams());
      
         // N.B. we do filtering here of which interactables allow this interactor to interact with them
         // We'd like to figure out a better system for this, since atm Aim Assist or other uses of UOSEInteractionHelpers::*
         // do not currently take this filtering into consideration
         if(found.GetObject() && _CanInteractWithTarget(ownerCharacter, found.GetObject()))
         {
            _lastStickyLocation = endPos;
            return found;
         }
      }
      const FVector toEnd = (endPos - startPos);
      const float initialLength = toEnd.Length();

      if(UseExtraDirectionalTraces)
      {
         // just a basic 4-tap for now
         const FRotator deltas[] = {
            FRotator(DirectionalTraceOffsetDegrees, 0.f, 0.f),
            FRotator(-DirectionalTraceOffsetDegrees, 0.f, 0.f),
            FRotator(0, DirectionalTraceOffsetDegrees, 0.f),
            FRotator(0, -DirectionalTraceOffsetDegrees, 0.f),
         };
         for(FRotator delta : deltas)
         {
            const FVector newEnd = startPos + (toEnd.Rotation() + delta).RotateVector(FVector::XAxisVector * initialLength);
            if (InteractCVars::DrawRange)
            {
               DrawDebugLine(GetWorld(), startPos, newEnd, FColor::Turquoise, false);
               DrawDebugSphere(GetWorld(), newEnd, 5, 10, FColor::Turquoise);
            }
            TScriptInterface<IInteractableInterface> found = UOSEInteractionHelpers::CastForInteractables(ownerCharacter, startPos, newEnd, _GetTraceResponseParams());
            if(found.GetObject() && _CanInteractWithTarget(ownerCharacter, found.GetObject()))
            {
               _lastStickyLocation = newEnd;
               return found;
            }
         }
      }

      // Try the last place it found something, if still close enough
      if(UseStickyTarget && _lastStickyLocation)
      {
         const float cosTolerance = FMath::Cos(FMath::DegreesToRadians(StickyTargetToleranceDegrees));
         const FVector stickyDir = (*_lastStickyLocation - startPos).GetSafeNormal();
         const float dotHeading = stickyDir | toEnd.GetSafeNormal();
         if(dotHeading > cosTolerance)
         {
            const FVector newEnd = startPos + (stickyDir * (initialLength + StickyTargetBonusRange));
            if (InteractCVars::DrawRange)
            {
               DrawDebugLine(GetWorld(), startPos, newEnd, FColor::Yellow, false);
               DrawDebugSphere(GetWorld(), newEnd, 5, 10, FColor::Yellow);
            }
            TScriptInterface<IInteractableInterface> found = UOSEInteractionHelpers::CastForInteractables(ownerCharacter, startPos, newEnd, _GetTraceResponseParams());
            if(found.GetObject() && _CanInteractWithTarget(ownerCharacter, found.GetObject()))
            {
               _lastStickyLocation = newEnd;
               return found;
            }
         }
      }
   }

   _lastStickyLocation = NullOpt;
   return nullptr;
}

bool UTATInteractionTargeterComponent::_CanInteractWithTarget(ACharacter* ownerCharacter, UObject* target) const
{
   if (InteractionFilterTag.IsValid())
   {
      return target->Implements<UTATSupportInteractionByInterface>()
         && ITATSupportInteractionByInterface::Execute_DoesSupportInteractionBy(target, ownerCharacter, InteractionFilterTag);
   }
   else
   {
      return true;
   }
}

const FCollisionResponseParams& UTATInteractionTargeterComponent::_GetTraceResponseParams() const
{
   return FCollisionResponseParams::DefaultResponseParam;
}

void UTATInteractionTargeterComponent::SetTargetingSuppressed(const bool suppressTargeting)
{
   const int32 delta = suppressTargeting ? 1 : -1;
   _suppressTargetingCounter = FMath::Max<int32>(_suppressTargetingCounter + delta, 0);
   _targetingSuppressed = _suppressTargetingCounter > 0;
}

void UTATInteractionTargeterComponent::RequestTargetingFreeze(const FName& source)
{
   _freezeTargetingRequests.AddUnique(source);
}

void UTATInteractionTargeterComponent::RemoveTargetingFreeze(const FName& source)
{
   _freezeTargetingRequests.RemoveSingleSwap(source);
}

void UTATInteractionTargeterComponent::ClearTargetingFreeze()
{
   _freezeTargetingRequests.Reset();
}


