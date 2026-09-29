// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

// self
#include "AI/Utility/UtilityAIStateTarget.h"

// ose

// ue
#include "SmartObjectComponent.h"
#include "SmartObjectSubsystem.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(UtilityAIStateTarget)

const FUtilityStateTarget FUtilityStateTarget::Invalid = FUtilityStateTarget();

FUtilityStateTarget::FUtilityStateTarget(AActor* actor)
   : Stim()
   , Actor(actor)
   , Component(nullptr)
   , SmartObjectRequestTarget()
   , TargetType(EBehaviorTargetType::Actor)
   , World(actor->GetWorld())
{
}

FUtilityStateTarget::FUtilityStateTarget(UActorComponent* component)
   : Stim()
   , Actor(nullptr)
   , Component(component)
   , SmartObjectRequestTarget()
   , TargetType(EBehaviorTargetType::ActorComponent)
   , World(component->GetWorld())
{
}

FUtilityStateTarget::FUtilityStateTarget(const FStimInfo& stim)
   : Stim(stim)
   , Actor(nullptr)
   , Component(nullptr)
   , SmartObjectRequestTarget()
   , TargetType(EBehaviorTargetType::Stim)
   , World(nullptr) // TODO?
{
}

FUtilityStateTarget::FUtilityStateTarget(const FSmartObjectRequestResult& smartObjectResult, UWorld* world, USmartObjectComponent* smartObjectComponent)
   : Stim()
   , Actor(nullptr)
   , Component(smartObjectComponent)
   , SmartObjectRequestTarget(smartObjectResult)
   , TargetType(EBehaviorTargetType::SmartObjectRequest)
   , World(world)
{
}

bool FUtilityStateTarget::PointsAt(AActor* actor) const
{
   return TargetType == EBehaviorTargetType::Actor && Actor.Get() == actor;
}

bool FUtilityStateTarget::PointsAt(UActorComponent* component) const
{
   return TargetType == EBehaviorTargetType::ActorComponent && Component.Get() == component;
}

bool FUtilityStateTarget::PointsAt(const FStimInfo& stim) const
{
   return TargetType == EBehaviorTargetType::Stim && Stim == stim;
}

bool FUtilityStateTarget::PointsAt(const FSmartObjectRequestResult& smartObjectResult) const
{
   return TargetType == EBehaviorTargetType::SmartObjectRequest && SmartObjectRequestTarget == smartObjectResult;
}

UObject* FUtilityStateTarget::GetTargetUObject() const
{
   switch (TargetType)
   {
      case EBehaviorTargetType::Actor:
         return Actor.Get();
         break;
      case EBehaviorTargetType::ActorComponent:
         return Component.Get();
         break;
      case EBehaviorTargetType::SmartObjectRequest:
         // the smart object component lookup is slow enough that is it passed into the target instead
         return Component.Get();
         break;
      case EBehaviorTargetType::Stim:
         return Stim.Instigator.Get();
         break;
   }
   return nullptr;
}

FVector FUtilityStateTarget::GetTargetWorldLocation() const
{
   switch (TargetType)
   {
   case EBehaviorTargetType::Actor:
      {
         if (AActor* actor = Actor.Get())
            return actor->GetActorLocation();
      }
      break;
   case EBehaviorTargetType::ActorComponent:
      {
         if (USceneComponent* sceneComp = Cast<USceneComponent>(Component.Get()))
            return sceneComp->GetComponentLocation();
      }
      break;
   case EBehaviorTargetType::Stim:
      {
         return Stim.Location;
      }
      break;
   case EBehaviorTargetType::SmartObjectRequest:
      {
         USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(World.Get());
         if (smartObjectSubsystem)
         {
            TOptional<FVector> slotLocation = smartObjectSubsystem->GetSlotLocation(SmartObjectRequestTarget);
            if (slotLocation.IsSet())
            {
               return slotLocation.GetValue();
            }
         }
      }
      break;
   }
   return FAISystem::InvalidLocation;
}

FString FUtilityStateTarget::ToString() const
{
   switch (TargetType)
   {
   case EBehaviorTargetType::Invalid:
      {
         return FString(TEXT("Invalid"));
      }
   case EBehaviorTargetType::Actor:
      {
         AActor* target = Actor.Get();
         return FString::Printf(TEXT("Actor: %s"), target ? *target->GetName() : TEXT("NULL"));
      }
   case EBehaviorTargetType::ActorComponent:
      {
         UActorComponent* target = Component.Get();
         return FString::Printf(TEXT("ActorComponent: %s"), target ? *target->GetName() : TEXT("NULL"));
      }
   case EBehaviorTargetType::Stim:
      {
         return FString::Printf(TEXT("Stim: %s"), *Stim.ToString());
      }
   case EBehaviorTargetType::SmartObjectRequest:
      if (UActorComponent* comp = Component.Get())
      {
         return FString::Printf(TEXT("Smart Object Component %s in Actor %s"), *comp->GetName(), comp->GetOwner() ? *comp->GetOwner()->GetName() : TEXT("<Unknown Actor>"));
      }
      else
      {
         return FString::Printf(TEXT("Smart Object Request %s in <Unknown Component>"), *LexToString(SmartObjectRequestTarget));
      }
   default:
      {
         checkNoEntry();
         return FString();
      }
   }
}

bool FUtilityStateTarget::operator==(const FUtilityStateTarget& other) const
{
   if (TargetType != other.TargetType)
   {
      return false;
   }

   switch (TargetType)
   {
   case EBehaviorTargetType::Invalid:
      {
         return true;
      }
   case EBehaviorTargetType::Actor:
      {
         return Actor == other.Actor;
      }
   case EBehaviorTargetType::ActorComponent:
      {
         return Component == other.Component;
      }
   case EBehaviorTargetType::Stim:
      {
         return Stim == other.Stim;
      }
   case EBehaviorTargetType::SmartObjectRequest:
      {
         return SmartObjectRequestTarget == other.SmartObjectRequestTarget;
      }
   default:
      {
         checkNoEntry();
         return false;
      }
   }
}

bool FUtilityStateTarget::operator!=(const FUtilityStateTarget& other) const
{
   return !operator==(other);
}

