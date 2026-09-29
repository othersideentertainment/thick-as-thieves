// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/Combat/TATCombatPositioningComponent.h"

// ue
#include "NavigationSystem.h"
#include "AI/TATAIController.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Navigation/PathFollowingComponent.h"
#include "NavFilters/NavigationQueryFilter.h"
#include "NavMesh/RecastNavMesh.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCombatPositioningComponent)
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Metadata_CombatPosition_Far, "AI.State.Metadata.CombatPosition.Far");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Metadata_CombatPosition_Mid, "AI.State.Metadata.CombatPosition.Mid");
UE_DEFINE_GAMEPLAY_TAG(TAG_AI_State_Metadata_CombatPosition_Close, "AI.State.Metadata.CombatPosition.Close");

namespace CombatPositioningCVars
{
   static int DebugDrawPositioning = 0;
   FAutoConsoleVariableRef CVarDebugDrawPositioning(
      TEXT("OSE.Combat.DebugDrawPositioning"),
      DebugDrawPositioning,
      TEXT("Draw combat positioning debug information"),
      ECVF_Default);
}

bool FCombatDirection::IsCombatDirectionAvailable(const float& minDistance, TWeakObjectPtr<const AOSECharacterBase> querier) const
{
   if(Distance < minDistance)
      return false;
   if(_CharacterUsingCombatDirection.IsValid() == false)
      return true;
   return _CharacterUsingCombatDirection == querier;
}

void FCombatDirection::SetOwnershipOfCombatDirection(TWeakObjectPtr<const AOSECharacterBase> querier)
{
   _CharacterUsingCombatDirection = querier;
}

float FCombatDirection::GetScoreForDirection(const FVector& querierDirectionToCharacter) const
{
   // Calculate dot product
   return  querierDirectionToCharacter | DirectionVector;
}

UTATCombatPositioningComponent::UTATCombatPositioningComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
}

void UTATCombatPositioningComponent::BeginPlay()
{
   Super::BeginPlay();
   float angleOffset = 0.f;
   const float angleToAdd = 360.f / _NumberOfDirections;
   while(angleOffset < 360.f)
   {
      FCombatDirection direction;
      direction.Angle = angleOffset;
      direction.DirectionVector = FVector::ForwardVector.RotateAngleAxis(direction.Angle, FVector::UpVector);
      _CombatDirections.Add(direction);
      angleOffset += angleToAdd;
   }

   MaxDistanceToCheck = 0.f;
   for (const TTuple<ECombatPosition, FCombatPositionData> combatPositionData : _CombatPositionData)
   {
      if (MaxDistanceToCheck < combatPositionData.Value.DistanceFromCharacter)
      {
         MaxDistanceToCheck = combatPositionData.Value.DistanceFromCharacter;
      }
   }
   
}

void UTATCombatPositioningComponent::_CalculateCombatPositions()
{
   const ACharacter* owner = Cast<ACharacter>(GetOwner());
   if(owner == nullptr)
      return;

   // Don't waste time calculating this if we have nobody to use it.
   if(_TargetingCharacterToCombatDirectionIndex.Num() == 0)
      return;
   
   const UCharacterMovementComponent* movementComponent = Cast<UCharacterMovementComponent>(owner->GetMovementComponent());
   if(movementComponent == nullptr)
      return;
   
   UNavigationSystemV1* navSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
   if (navSys == nullptr)
   {
      return;
   }

   ANavigationData* navData = _GetNavigationData(*navSys, owner);
   const ARecastNavMesh* navMeshData = Cast<ARecastNavMesh>(navData);
   if (navMeshData == nullptr)
   {
      return;
   }
   FNavLocation location;
   const FSharedConstNavQueryFilter navFilter = UNavigationQueryFilter::GetQueryFilter(*navData, _NavigationFilter);
   if(movementComponent->FindNavFloor(owner->GetNavAgentLocation(), location))
   {
      for (FCombatDirection& combatDirection : _CombatDirections)
      {
         FVector targetLocation = owner->GetNavAgentLocation() + combatDirection.DirectionVector * MaxDistanceToCheck;
#if ENABLE_DRAW_DEBUG
         if(CombatPositioningCVars::DebugDrawPositioning != 0)
         {
            DrawDebugLine(GetWorld(), location, combatDirection.DirectionEndPosition, FColor::Magenta, false, 0.01f, 0, 1.f);
         }
#endif  
         FVector hitLocation;
         // We don't care if this returns true or not, either way it'll return a valid hit location (either start location OR the actual hit location)
         ARecastNavMesh::NavMeshRaycastWithHeight(navMeshData, location, targetLocation, hitLocation, navFilter, owner);   
         combatDirection.Distance = FVector::Distance(location, hitLocation);
         combatDirection.DirectionEndPosition = hitLocation;
      }
   }
}

bool UTATCombatPositioningComponent::_AssignCombatPositionForCharacter(
   const TWeakObjectPtr<const AOSECharacterBase> weakCharacter)
{
   const int currentValue = _TargetingCharacterToCombatDirectionIndex[weakCharacter];
   const AOSECharacterBase* character = weakCharacter.Get();
   if (character == nullptr)
   {
      return false;
   }
   
   float bestScore = -1.f;
   int bestIndex = INDEX_NONE;
   const FVector direction = (character->GetActorLocation()-GetOwner()->GetActorLocation()).GetSafeNormal2D();
   ECombatPosition combatPosition = ECombatPosition::Close;
   if (ATATAIController* controller = character->GetController<ATATAIController>())
   {
      combatPosition = controller->GetRequestedCombatPosition();
   }
   float minDistanceToBeValid = 0.f;
   if(const FCombatPositionData* positionData = _CombatPositionData.Find(combatPosition))
   {
      minDistanceToBeValid = positionData->DistanceFromCharacter;
   }
   for (int i=0; i < _CombatDirections.Num(); ++i)
   {
      FCombatDirection& combatDirection = _CombatDirections[i];
      if(combatDirection.IsCombatDirectionAvailable(minDistanceToBeValid, character))
      {
         const float score = combatDirection.GetScoreForDirection(direction);
         if(score > bestScore)
         {
            bestIndex = i;
            bestScore = score;
         }
      }
   }
   if(currentValue != bestIndex)
   {
      if(currentValue != INDEX_NONE)
      {
         _CombatDirections[currentValue].SetOwnershipOfCombatDirection(nullptr);
      }
      if(bestIndex != INDEX_NONE)
      {
         _CombatDirections[bestIndex].SetOwnershipOfCombatDirection(weakCharacter);
      }
      _TargetingCharacterToCombatDirectionIndex[weakCharacter] = bestIndex;
   }
   return true;
}


void UTATCombatPositioningComponent::TickComponent(float deltaTime,
                                                   ELevelTick tickType,
                                                   FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   _CalculateCombatPositions();
   for( auto it = _TargetingCharacterToCombatDirectionIndex.CreateIterator(); it; ++it)
   {
      if (!_AssignCombatPositionForCharacter(it.Key()))
      {
         // If character is gone, but not un-registered, remove it now
         // Ultimately FTATStateTreeTaskSetCombatTarget::ExitState is not called when NPC actor is destroyed, which should also be fixed
         if(it.Value() != INDEX_NONE && _CombatDirections.IsValidIndex(it.Value()))
         {
            _CombatDirections[it.Value()].SetOwnershipOfCombatDirection(nullptr);
         }
         it.RemoveCurrent();
      }
   }
#if ENABLE_DRAW_DEBUG
   if(CombatPositioningCVars::DebugDrawPositioning != 0)
   {
      if(_TargetingCharacterToCombatDirectionIndex.Num() != 0)
      {
         const FVector ownerPosition = GetOwner()->GetActorLocation();
         for (FCombatDirection& combatDirection : _CombatDirections)
         {
            if(combatDirection.HasOwner())
            {
               DrawDebugLine(GetWorld(), ownerPosition, combatDirection.DirectionEndPosition, FColor::Emerald, false, 0.01f, 0, 5.f);
            }
            else
            {
               DrawDebugLine(GetWorld(), ownerPosition, combatDirection.DirectionEndPosition, FColor::Yellow, false, 0.01f, 0, 1.f);
            }
         }
      }
   }
#endif
}

void UTATCombatPositioningComponent::AddTargetingCharacter(AOSECharacterBase* character)
{
   _TargetingCharacterToCombatDirectionIndex.Add(character, INDEX_NONE);   
}

void UTATCombatPositioningComponent::RemoveTargetingCharacter(AOSECharacterBase* character)
{
   if(const int* indexPointer = _TargetingCharacterToCombatDirectionIndex.Find(character))
   {
      if(indexPointer)
      {
         const int index = *indexPointer;
         if(_CombatDirections.IsValidIndex(index))
         {
            _CombatDirections[*indexPointer].SetOwnershipOfCombatDirection(nullptr);
         }
      }
   }
   _TargetingCharacterToCombatDirectionIndex.Remove(character);   
}

FVector UTATCombatPositioningComponent::GetOffsetForCharacter(const AOSECharacterBase* character, const ECombatPosition position) const
{
   if(const int* indexPointer = _TargetingCharacterToCombatDirectionIndex.Find(character))
   {
      if(indexPointer)
      {
         const int index = *indexPointer;
         if(_CombatDirections.IsValidIndex(index))
         {
            const FCombatDirection& direction = _CombatDirections[*indexPointer];
            float offset = direction.Distance;
            if(const FCombatPositionData* positionData = _CombatPositionData.Find(position))
            {
               offset = positionData->DistanceFromCharacter;
            }
#if ENABLE_DRAW_DEBUG
            if(CombatPositioningCVars::DebugDrawPositioning != 0)
            {
               const FVector ownerPosition = GetOwner()->GetActorLocation();
               DrawDebugSphere(GetWorld(),
                  ownerPosition + (direction.DirectionVector * offset),
                  25.f,
                  4,
                  FColor::Red,
                  false,
                  0.01f,
                  0,
                  1.f);
            }
#endif
            return direction.DirectionVector * offset;
         }
      }
   }
   
#if ENABLE_DRAW_DEBUG
   if(CombatPositioningCVars::DebugDrawPositioning != 0)
   {
      const FVector ownerPosition = GetOwner()->GetActorLocation();
      DrawDebugSphere(GetWorld(),
         ownerPosition,
         25.f,
         4,
         FColor::Blue,
         false,
         0.01f,
         0,
         1.f);
   }
#endif
   return FVector::ZeroVector;
}

ANavigationData* UTATCombatPositioningComponent::_GetNavigationData(UNavigationSystemV1& navSys,
                                                                    const AActor* actor)
{
   if (const INavAgentInterface* navAgent = Cast<INavAgentInterface>(actor))
   {
      return navSys.GetNavDataForProps(navAgent->GetNavAgentPropertiesRef(), navAgent->GetNavAgentLocation());
   }
   return navSys.GetDefaultNavDataInstance(FNavigationSystem::DontCreate);
}

