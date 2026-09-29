// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// tat
#include "Variation/SceneVariants/TATSceneRequirement.h"
#include "GameFramework/TATEndgameReason.h"

// ue
#include "Components/ActorComponent.h"

#include "TATEndgameActionComponent.generated.h"


UENUM()
enum class ETATDefaultEndgameAction : uint8
{
   None,
   TurnOn,
   TurnOff,
   TriggerTrapAction
};

// A convenience component for listening for the end-game starting
// for specific reasons
// 
// (Could also be a more generic event bus, but this works for now)
//
// NOTE: This is authority-only right now, as that is the initial
//       use-case, but it would be straightforward to convert it
//       later if the GameState replicates its endgame reasons.
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TAT_API UTATEndgameActionComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   // Sets default values for this component's properties
   UTATEndgameActionComponent();

   void SetDefaultAction(ETATDefaultEndgameAction action) { _defaultAction = action; }
   
   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEndgameActionTriggered);
   UPROPERTY(BlueprintAssignable)
   FOnEndgameActionTriggered OnAuthorityEndgameActionTriggered;

#if WITH_EDITOR
   virtual void CheckForErrors() override final;

   // for vis adapter
   const FTATSceneRequirement* FindSceneRequirement() const;
#endif

protected:
   // Called when the game starts
   virtual void BeginPlay() override;


private:
   void _OnEndgameReasonAdded(ETATEndgameReason reason);
   void _TriggerEndgameAction();
   
   UPROPERTY(EditAnywhere, Category=Endgame)
   bool _triggerOnEndgame = false;

   // Scene requirement for triggering actions based on game-state
   UPROPERTY(EditAnywhere, Category=Endgame)
   FTATSceneRequirement _sceneRequirement;

   UPROPERTY(EditDefaultsOnly, Category=Endgame, meta = (InlineEditConditionToggle))
   bool _requireSpecificReason = true;

   UPROPERTY(EditDefaultsOnly, Category=Endgame, meta = (editcondition="_requireSpecificReason"))
   ETATEndgameReason _requiredReason = ETATEndgameReason::Mission;

   // Some convenience builtin actions
   UPROPERTY(EditAnywhere, Category=Endgame)
   ETATDefaultEndgameAction _defaultAction = ETATDefaultEndgameAction::None;

   bool _hasTriggered = false;
};
