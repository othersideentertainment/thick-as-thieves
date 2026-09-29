// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CoreMinimal.h"

// tat
#include "Player/TATPlayerState.h"

#include "TATPlayerPerceivableComponent.generated.h"

UENUM(BlueprintType)
enum class ETATPlayerPerceptionLevel : uint8
{
   None,
   Perceived,
   Aware,
   Focused,
   Fixated,
   Count UMETA(Hidden)
};


/// A component representing something that can be perceived by the player
/// The perception state is local-only, with a best-effort RPC sent to the server
/// so it can track the perception levels of each player
UCLASS(Blueprintable, meta = (BlueprintSpawnableComponent))
class TAT_API UTATPlayerPerceivableComponent : public UActorComponent
{
   GENERATED_BODY()
public:
   UTATPlayerPerceivableComponent();

   virtual void BeginPlay() override;
   virtual void EndPlay(EEndPlayReason::Type reason) override;
#if WITH_EDITOR
   virtual EDataValidationResult IsDataValid(FDataValidationContext& context) const override;
#endif

   UFUNCTION(BlueprintPure)
   ETATPlayerPerceptionLevel GetCurrentPerceptionLevel() const { return _perceptionLevel; }

   /// Only fires locally, based on the local player's perception level
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLocalPerceptionLevelChanged, ETATPlayerPerceptionLevel, newPerceptionLevel);
   UPROPERTY(BlueprintAssignable)
   FOnLocalPerceptionLevelChanged OnLocalPerceptionLevelChanged;

   /// Only fires on the server, includes which player the perception change is for
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FAuthorityOnPerceptionLevelForPlayerChanged, ATATPlayerState*, playerState, ETATPlayerPerceptionLevel, newPerceptionLevel);
   UPROPERTY(BlueprintAssignable)
   FAuthorityOnPerceptionLevelForPlayerChanged AuthorityOnPerceptionLevelForPlayerChanged;

   ETATPlayerPerceptionLevel AuthorityGetCurrentPerceptionLevelForPlayer(ATATPlayerState* playerState) const;

   void SetCurrentPerceptionLevel(ETATPlayerPerceptionLevel perceptionLevel);

   /// Points in actor's local space that should be checked for visibility
   /// Default is a single point in the centre of the actor
   UPROPERTY(EditAnywhere, Category = Perceivable)
   TArray<FVector> VisibilityCheckPoints;

   /// How quickly should the perception system focus on this actor
   UPROPERTY(EditAnywhere, Category = Perceivable)
   float ImportanceFactor = 1.0f;

private:
   // TODO: We should re-evaluate if this should be reliable or unreliable
   UFUNCTION(Server, Unreliable)
   void _ServerSetCurrentPerceptionLevelForPlayer(ATATPlayerState* playerState, ETATPlayerPerceptionLevel perceptionLevel);

   UFUNCTION()
   void _OnLocalPlayerStateChanged(APlayerState* ps);

   bool _isWaitingForPlayerState = false;

   ETATPlayerPerceptionLevel _perceptionLevel = ETATPlayerPerceptionLevel::None;

   TMap<TWeakObjectPtr<ATATPlayerState>, ETATPlayerPerceptionLevel> _authorityPerPlayerPerceptions;
};

