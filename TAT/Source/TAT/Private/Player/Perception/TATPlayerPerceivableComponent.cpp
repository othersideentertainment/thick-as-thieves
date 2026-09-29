// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Player/Perception/TATPlayerPerceivableComponent.h"

// tat
#include "Player/TATPlayerController.h"
#include "Player/Perception/TATPlayerPerceptionSubsystem.h"

// ue
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerPerceivableComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATPlayerPerceivableComponent, Log, All)

UTATPlayerPerceivableComponent::UTATPlayerPerceivableComponent()
{
   VisibilityCheckPoints.Add(FVector::ZeroVector);
}

void UTATPlayerPerceivableComponent::BeginPlay()
{
   Super::BeginPlay();

   GetWorld()->GetSubsystem<UTATPlayerPerceptionSubsystem>()->RegisterPerceivable(this);
}

void UTATPlayerPerceivableComponent::EndPlay(EEndPlayReason::Type reason)
{
   GetWorld()->GetSubsystem<UTATPlayerPerceptionSubsystem>()->UnregisterPerceivable(this);

   Super::EndPlay(reason);
}

#if WITH_EDITOR
EDataValidationResult UTATPlayerPerceivableComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (VisibilityCheckPoints.Num() == 0)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has no Visibility Check Points, but needs at least one"), *GetName())));
      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif

ETATPlayerPerceptionLevel UTATPlayerPerceivableComponent::AuthorityGetCurrentPerceptionLevelForPlayer(ATATPlayerState* playerState) const
{
   if (const ETATPlayerPerceptionLevel* perceptionLevel = _authorityPerPlayerPerceptions.Find(playerState))
   {
      return *perceptionLevel;
   }

   // If we aren't tracking one, default to None
   return ETATPlayerPerceptionLevel::None;
}

void UTATPlayerPerceivableComponent::SetCurrentPerceptionLevel(ETATPlayerPerceptionLevel perceptionLevel)
{
   if (perceptionLevel != _perceptionLevel)
   {
      // Set local state
      _perceptionLevel = perceptionLevel;

      // BP callback for local change in perception level
      OnLocalPerceptionLevelChanged.Broadcast(perceptionLevel);

      // Forward the change to the global event on the subsystem
      GetWorld()->GetSubsystem<UTATPlayerPerceptionSubsystem>()->OnLocalPerceptionLevelChangedForPerceivable.Broadcast(this, perceptionLevel);

      // Let the server know about the change
      if (ATATPlayerController* pc = ATATPlayerController::GetLocalTATPlayerController(GetWorld()))
      {
         if (ATATPlayerState* ps = pc->GetTATPlayerState())
         {
            _ServerSetCurrentPerceptionLevelForPlayer(ps, perceptionLevel);
         }
         else
         {
            // If we don't yet have a player state, bind to the event for when it's set
            // If we're already waiting for one, don't bind a second time: we will take the most recent perception level
            // in _OnLocalPlayerStateChanged
            if (!_isWaitingForPlayerState)
            {
               pc->OnPlayerStateChanged.AddUniqueDynamic(this, &UTATPlayerPerceivableComponent::_OnLocalPlayerStateChanged);
            }
         }
      }
      else
      {
         UE_LOG(LogTATPlayerPerceivableComponent, Warning,
            TEXT("Calling SetCurrentPerceptionLevel() on '%s', despite not having a local player controller"),
            *GetNameSafe(GetOwner()));
      }
   }
}

void UTATPlayerPerceivableComponent::_ServerSetCurrentPerceptionLevelForPlayer_Implementation(ATATPlayerState* playerState, ETATPlayerPerceptionLevel perceptionLevel)
{
   _authorityPerPlayerPerceptions.FindOrAdd(playerState) = perceptionLevel;

   AuthorityOnPerceptionLevelForPlayerChanged.Broadcast(playerState, perceptionLevel);
}

void UTATPlayerPerceivableComponent::_OnLocalPlayerStateChanged(APlayerState* ps)
{
   // Send the server our latest perception level
   _ServerSetCurrentPerceptionLevelForPlayer(Cast<ATATPlayerState>(ps), _perceptionLevel);

   _isWaitingForPlayerState = false;

   // We now have a player state, so we don't need further events
   if (ATATPlayerController* pc = ATATPlayerController::GetLocalTATPlayerController(GetWorld()))
   {
      pc->OnPlayerStateChanged.RemoveAll(this);
   }
}

