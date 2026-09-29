// (c) 2018-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Patrol/PatrolPath.h"

// tat
#include "AI/Patrol/PatrolPathComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(PatrolPath)

int32 GetNextValidPoint(int32 currentPoint, bool& direction, int32 totalNumberOfPoints, EPatrolLoopType loopType)
{
   switch (loopType)
   {
   case EPatrolLoopType::Once:
      {
         const int nextIndex = currentPoint + 1;
         if(totalNumberOfPoints == 0 || nextIndex < 0)
         {
            return -1;
         }
         if(nextIndex >= totalNumberOfPoints)
         {
            return currentPoint;
         }
         return nextIndex;
      }
   case EPatrolLoopType::PingPong:
      {
         int nextIndex = direction ? currentPoint + 1 : currentPoint - 1;
         if(totalNumberOfPoints == 0)
         {
            return -1;
         }
         if(totalNumberOfPoints == 1)
         {
            return currentPoint;
         }
         if(nextIndex >= totalNumberOfPoints || nextIndex < 0)
         {
            direction = !direction;
            nextIndex = direction ? currentPoint + 1 : currentPoint - 1; 
            return nextIndex;
         }
         return nextIndex;
      }
   case EPatrolLoopType::Cycle:
   default:
      {
         const int nextIndex = currentPoint + 1;
         if(totalNumberOfPoints == 0)
         {
            return -1;
         }
         if(totalNumberOfPoints == 1)
         {
            return currentPoint;
         }
         if(nextIndex < totalNumberOfPoints)
         {
            return nextIndex;
         }
         return 0;
      }
   }
}

AWatchPath::AWatchPath()
{
   bNetLoadOnClient = false;

   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
   RootComponent->SetVisibleFlag(false);
   RootComponent->Mobility = EComponentMobility::Static;

#if WITH_EDITOR
   PathComponent = CreateEditorOnlyDefaultSubobject<UPatrolPathComponent>(TEXT("PathComponent"));
   if (PathComponent)
   {
      PathComponent->SetupAttachment(RootComponent);
   }
#endif
}

#if WITH_EDITOR
void AWatchPath::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   if (PathComponent)
   {
      PathComponent->MarkRenderStateDirty();
   }
}
#endif

int32 AWatchPath::GetNumPoints() const
{
   return Points.Num();
}

FVector AWatchPath::GetPointLocationLocalSpace(int32 pointIndex) const
{
   if (pointIndex >= Points.Num())
   {
      return FVector::ZeroVector;
   }
   return Points[pointIndex].LookAtPosition;
}

FVector AWatchPath::GetPointLocationWorldSpace(int32 pointIndex) const
{
   if (pointIndex >= Points.Num())
   {
      return FVector::ZeroVector;
   }

   return ActorToWorld().TransformPosition(GetPointLocationLocalSpace(pointIndex));
}

FNextPointData AWatchPath::GetNextPoint(int32 currentPoint, bool currentDirection) const
{
   FNextPointData data;
   data.forwardMovementDirection = currentDirection;
   data.nextIndexID = GetNextValidPoint(currentPoint, data.forwardMovementDirection, Points.Num(), LoopType);
   return data;
}


#if WITH_EDITOR
FColor AWatchPath::GetColorForPoint(int32 pointIndex) const
{
   return FColor::White;
}
#endif

APatrolPath::APatrolPath()
{
   bNetLoadOnClient = false;

   RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("SceneRoot"));
   RootComponent->SetVisibleFlag(false);
   RootComponent->Mobility = EComponentMobility::Static;

#if WITH_EDITOR
   PathComponent = CreateEditorOnlyDefaultSubobject<UPatrolPathComponent>(TEXT("PathComponent"));
   if (PathComponent)
   {
      PathComponent->SetupAttachment(RootComponent);
   }
#endif
}

#if WITH_EDITOR
void APatrolPath::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   if (PathComponent)
   {
      PathComponent->MarkRenderStateDirty();
   }
}

void APatrolPath::PostEditChangeChainProperty(FPropertyChangedChainEvent& propertyChangedEvent)
{
   Super::PostEditChangeChainProperty(propertyChangedEvent);
   if (const FProperty* propertyThatChanged = propertyChangedEvent.Property)
   {
      if(propertyChangedEvent.ChangeType == EPropertyChangeType::ArrayAdd)
      {
         const int32 index = propertyChangedEvent.GetArrayIndex(TEXT("Points"));
         if(Points.IsValidIndex(index) && Points.IsValidIndex(index-1))
         {
            Points[index].Position = Points[index-1].Position + FVector(100,100,100);
         }
         else if(Points.IsValidIndex(index))
         {
            Points[index].Position = FVector(100,100,100);
         }
         return;
      }
      if (propertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED(FPatrolPoint, SmartObjectComponent) ||
         propertyThatChanged->GetFName() == GET_MEMBER_NAME_CHECKED(FPatrolPoint, SlotIndex))
      {
         const int32 index = propertyChangedEvent.GetArrayIndex(TEXT("Points"));
         if(Points.IsValidIndex(index))
         {
            if(const ATATAmbientSmartObject* so = Points[index].SmartObjectComponent)
            {
               if(const UTATSmartObjectComponent* soComponent = so->GetSmartObjectComponent())
               {
                  const FTransform OwnerLocalToWorld = soComponent->GetComponentTransform();
                  if(const USmartObjectDefinition* definition = soComponent->GetDefinition())
                  {
                     if(definition->GetSlots().IsValidIndex(Points[index].SlotIndex))
                     {
                        TOptional<FTransform> transform = definition->GetSlotWorldTransform(Points[index].SlotIndex, OwnerLocalToWorld);
                        if(transform.IsSet())
                        {
                           Points[index].Position = GetTransform().InverseTransformPosition(transform.GetValue().GetLocation()); 
                        }
                     }
                  }
               }
            }
         }
      }
   }
}


FColor APatrolPath::GetColorForPoint(int32 pointIndex) const
{
   if (Points.IsValidIndex(pointIndex))
   {
      switch(Points[pointIndex].PatrolPointType)
      {
      case EPatrolPointType::Interact:
         return FColor::Blue;
      case EPatrolPointType::WatchPath:
         return FColor::Yellow;
      case EPatrolPointType::PatrolToLocation:
         return FColor::White;
      case EPatrolPointType::SmartObjectInteraction:
         return FColor::Orange;
      }
   }
   return FColor::White;
}

#endif

FNextPointData APatrolPath::GetNextPoint(int32 currentPoint, bool currentDirection) const
{
   FNextPointData data;
   data.forwardMovementDirection = currentDirection;
   data.nextIndexID = GetNextValidPoint(currentPoint, data.forwardMovementDirection, Points.Num(), LoopType);
   return data;
}
int32 APatrolPath::GetNumPoints() const
{
   return Points.Num();
}

FVector APatrolPath::GetPointLocationLocalSpace(int32 pointIndex) const
{
   if (pointIndex >= Points.Num())
   {
      return FVector::ZeroVector;
   }
   return Points[pointIndex].Position;
}

FVector APatrolPath::GetPointLocationWorldSpace(int32 pointIndex) const
{
   if (pointIndex >= Points.Num())
   {
      return FVector::ZeroVector;
   }

   return ActorToWorld().TransformPosition(GetPointLocationLocalSpace(pointIndex));
}
