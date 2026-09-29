// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "GameFramework/TATEndgameActionComponent.h"

// tat
#include "Variation/SceneVariants/TATSceneVariantUtils.h"
#include "Online/TATGameState.h"
#include "Traps/TATTrapActionInterface.h"

// ose
#include "Interactables/OSEToggleInterface.h"

// ue
#include "Misc/UObjectToken.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(TATEndgameActionComponent)

// Sets default values for this component's properties
UTATEndgameActionComponent::UTATEndgameActionComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

#if WITH_EDITOR
void UTATEndgameActionComponent::CheckForErrors()
{
   Super::CheckForErrors();

   FMessageLog messageLog("MapCheck");
   _sceneRequirement.ValidateRequirement(messageLog, GetOwner(), [this] { return FUObjectToken::Create(GetOwner(), FText::FromString(GetReadableName())); });
}

const FTATSceneRequirement* UTATEndgameActionComponent::FindSceneRequirement() const
{
   if (_triggerOnEndgame)
   {
      return &_sceneRequirement;
   }

   return nullptr;
}
#endif // WITH_EDITOR

// Called when the game starts
void UTATEndgameActionComponent::BeginPlay()
{
   Super::BeginPlay();

   if(!GetOwner()->HasAuthority() || !_triggerOnEndgame)
   {
      return;
   }

   if (!_sceneRequirement.IsNone() && !UTATSceneVariantUtils::ResolveBoolRequirement(GetWorld(), _sceneRequirement))
   {
      return;
   }

   ATATGameState* gameState = GetWorld()->GetGameState<ATATGameState>();
   if(gameState == nullptr)
   {
      return;
   }

   if(_requireSpecificReason ? gameState->AuthorityHasEndGameReason(_requiredReason)
      : gameState->GetCurrentPhase() == ETATMatchPhase::Endgame)
   {
      _TriggerEndgameAction();
   }
   else
   {
      gameState->OnAuthorityEndgameReasonAdded.AddUObject(this, &UTATEndgameActionComponent::_OnEndgameReasonAdded);
   }
}

void UTATEndgameActionComponent::_OnEndgameReasonAdded(ETATEndgameReason reason)
{
   if(_requiredReason == reason || !_requireSpecificReason)
   {
      _TriggerEndgameAction();
   }
}

void UTATEndgameActionComponent::_TriggerEndgameAction()
{
   if(_hasTriggered)
   {
      return;
   }
   
   _hasTriggered = true;

   OnAuthorityEndgameActionTriggered.Broadcast();

   switch (_defaultAction)
   {
   case ETATDefaultEndgameAction::TurnOff:
   case ETATDefaultEndgameAction::TurnOn:
      if(IOSEToggleInterface* toggle = Cast<IOSEToggleInterface>(GetOwner()))
      {
         toggle->SetToggleOn(_defaultAction == ETATDefaultEndgameAction::TurnOn);
      }
      break;
   case ETATDefaultEndgameAction::TriggerTrapAction:
      if(GetOwner()->Implements<UTATTrapActionInterface>())
      {
         ITATTrapActionInterface::Execute_TriggerActionFromTrap(GetOwner(), nullptr);
      }
   default:
      break;
   }
}

