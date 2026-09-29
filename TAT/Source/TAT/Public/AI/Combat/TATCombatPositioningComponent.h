// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "NavigationSystem.h"
#include "Components/ActorComponent.h"

// tat
#include "AI/Alertness/TATAlertnessGameplayTags.h"
#include "Character/TATCharacterAIBase.h"

#include "TATCombatPositioningComponent.generated.h"

class ATATCharacterAIBase;

USTRUCT()
struct FCombatDirection
{
   GENERATED_BODY()

   float Angle { 0.f };
   float Distance { 0.f };
   FVector DirectionVector { FVector::ZeroVector };
   FVector DirectionEndPosition { FVector::ZeroVector };
   bool IsValid { false };

   bool IsCombatDirectionAvailable(const float& minDistance, TWeakObjectPtr<const AOSECharacterBase> querier) const;
   void SetOwnershipOfCombatDirection(TWeakObjectPtr<const AOSECharacterBase> querier);
   float GetScoreForDirection(const FVector& querierDirectionToCharacter) const;
   bool HasOwner() const { return _CharacterUsingCombatDirection.IsValid(); };

private:
   TWeakObjectPtr<const AOSECharacterBase> _CharacterUsingCombatDirection;
};

UENUM(BlueprintType)
enum class ECombatPosition : uint8
{
   Close,
   Mid,
   Far
};

UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_State_Metadata_CombatPosition_Far);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_State_Metadata_CombatPosition_Mid);
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_AI_State_Metadata_CombatPosition_Close);

USTRUCT()
struct FCombatPositionData
{
   GENERATED_BODY()

   UPROPERTY(EditDefaultsOnly)
   float DistanceFromCharacter { 100.f };
};

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATCombatPositioningComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATCombatPositioningComponent();
   virtual void BeginPlay() override;

   virtual void TickComponent(float deltaTime,
                              ELevelTick tickType,
                              FActorComponentTickFunction* thisTickFunction) override;
   
   void AddTargetingCharacter(AOSECharacterBase* character);
   void RemoveTargetingCharacter(AOSECharacterBase* character);

   FVector GetOffsetForCharacter(const AOSECharacterBase* character, const ECombatPosition position) const;
protected:
   static ANavigationData* _GetNavigationData(UNavigationSystemV1& navSys, const AActor* actor);
   void _CalculateCombatPositions();
   bool _AssignCombatPositionForCharacter(TWeakObjectPtr<const AOSECharacterBase> character);

   UPROPERTY(EditDefaultsOnly, Category=Trace)
   TSubclassOf<UNavigationQueryFilter> _NavigationFilter;
   
   // We're going to divide 360 by this number, and you'll have that number of positions valid to attack from
   UPROPERTY(EditDefaultsOnly)
   int _NumberOfDirections { 8 };

   UPROPERTY(EditDefaultsOnly)
   TMap<ECombatPosition, FCombatPositionData> _CombatPositionData {
      {
            ECombatPosition::Close, { 100.f }
         },
      {
            ECombatPosition::Mid, { 500.f }
         },
      {
            ECombatPosition::Far, { 1000.f }
         },      
   }; 

   UPROPERTY(Transient)
   TArray<FCombatDirection> _CombatDirections;

   UPROPERTY(Transient)
   TMap<TWeakObjectPtr<const AOSECharacterBase>, int> _TargetingCharacterToCombatDirectionIndex;
   float MaxDistanceToCheck { 0.f };
};
