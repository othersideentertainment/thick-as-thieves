// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Tools/TATReticleStateComponent.h"

// tat
#include "Tools/TATCustomReticleInterface.h"
#include "Tools/TATCustomReticleWidget.h"

// ose
#include "Items/ToolComponent.h"
#include "Items/ToolSetInterface.h"
#include "Items/ToolSetSystemInterface.h"
#include "Player/OSEPlayerController.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/Controller.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATReticleStateComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATReticleStateComponent, Log, All)

UTATReticleStateComponent::UTATReticleStateComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UTATReticleStateComponent::BeginPlay()
{
   Super::BeginPlay();

   if (AOSEPlayerController* controller = Cast<AOSEPlayerController>(GetOwner()))
   {
      // For now, we only want to tick if we are a local player, since we're dealing with reticle state
      if (controller->IsLocalPlayerController())
      {
         SetComponentTickEnabled(true);

         // Track our pawn so when it changes we know to remove loose tags and reset our reticle state
         _currentlyPossessedPawn = controller->GetPawn();
         controller->OnPawnChanged.AddUniqueDynamic(this, &UTATReticleStateComponent::_OnOwnerPawnChanged);
      }
   }
}


void UTATReticleStateComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   // If we don't have a tool equipped, or it doesn't implement the custom reticle interface,
   // set the reticle tag back to empty
   FGameplayTag newReticleTag = FGameplayTag::EmptyTag;
   AActor* newReticleTargetActor = nullptr;
   TSubclassOf<UTATCustomReticleWidgetBase> newWidget = nullptr;

   if (UToolComponent* currentTool = _GetCurrentlyEquippedTool())
   {
      if (currentTool->Implements<UTATCustomReticleInterface>())
      {
         newWidget = ITATCustomReticleInterface::Execute_GetReticleWidgetClass(currentTool);
         newReticleTag = ITATCustomReticleInterface::Execute_GetCurrentReticleState(currentTool);
         newReticleTargetActor = ITATCustomReticleInterface::Execute_GetCurrentReticleTargetActor(currentTool);
      }
   }

   // Set everything before firing events, so listeners can poll without getting inconsistent stuff
   FGameplayTag oldReticleTag = _currentReticleTag;
   AActor* oldReticleTargetActor = _currentReticleTargetActor;
   TSubclassOf<UTATCustomReticleWidgetBase> oldWidget = _currentReticleWidgetClass;

   _currentReticleTag = newReticleTag;
   _currentReticleTargetActor = newReticleTargetActor;
   _currentReticleWidgetClass = newWidget;

   _MaybeNotifyNewReticleWidget(oldWidget);
   _MaybeNotifyNewReticleTag(oldReticleTag);
   _MaybeNotifyNewReticleTargetActor(oldReticleTargetActor);
}

FGameplayTag UTATReticleStateComponent::GetCurrentReticleState() const
{
   return _currentReticleTag;
}

AActor* UTATReticleStateComponent::GetCurrentReticleTargetActor() const
{
   return _currentReticleTargetActor;
}

TSubclassOf<UTATCustomReticleWidgetBase> UTATReticleStateComponent::GetCurrentReticleWidgetClass() const
{
   return _currentReticleWidgetClass;
}

void UTATReticleStateComponent::_OnOwnerPawnChanged(APawn* newPawn)
{
   // If we're unpossessing a pawn, we need to remove any loose tag we gave it for the reticle
   // N.B. This currently assumes that the ASC is still on the old pawn by the time we get the unpossess callback
   // If we have more complicated relationships b/w pawns and ASC's, we may need additional logic that
   // maintains the tags in a smarter way
   if (APawn* currentPawn = _currentlyPossessedPawn.Get())
   {
      if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(currentPawn))
      {
         if (_currentReticleTag.IsValid())
         {
            asc->RemoveLooseGameplayTag(_currentReticleTag);
         }
      }
   }

   _currentlyPossessedPawn = newPawn;

   // Reset the reticle tag so that the new pawn will pick up any changes
   // from a known starting point
   _currentReticleTag = FGameplayTag::EmptyTag;
}

UToolComponent* UTATReticleStateComponent::_GetCurrentlyEquippedTool()
{
   const APawn* pawn = _currentlyPossessedPawn.Get();

   if (const IGameplayTagAssetInterface* tagInterface = Cast<IGameplayTagAssetInterface>(pawn))
   {
      if (tagInterface->HasAnyMatchingGameplayTags(_ignoreToolTags))
      {
         return nullptr;
      }
   }

   if (const IToolSetSystemInterface* pawnToolSetSystemInterface = Cast<IToolSetSystemInterface>(pawn))
   {
      if (TScriptInterface<IToolSetInterface> toolSetInterface = pawnToolSetSystemInterface->GetToolSetInterface())
      {
         return toolSetInterface->GetCurrentTool();
      }
   }

   return nullptr;
}

void UTATReticleStateComponent::_MaybeNotifyNewReticleWidget(TSubclassOf<UTATCustomReticleWidgetBase> oldWidget)
{
   if (oldWidget != _currentReticleWidgetClass)
   {
      OnReticleWidgetChanged.Broadcast(_currentReticleWidgetClass);
   }
}

void UTATReticleStateComponent::_MaybeNotifyNewReticleTag(FGameplayTag oldReticleTag)
{
   FGameplayTag newReticleTag = _currentReticleTag;
   if (oldReticleTag != newReticleTag)
   {
      UE_LOG(LogTATReticleStateComponent, Verbose, TEXT("Custom tool reticle state changed from '%s' to '%s'"),
         *oldReticleTag.ToString(), *newReticleTag.ToString());

      OnReticleStateChanged.Broadcast(oldReticleTag, newReticleTag);

      if (APawn* currentPawn = _currentlyPossessedPawn.Get())
      {
         if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(currentPawn))
         {
            // Remove the old reticle loose tag if there was one
            if (oldReticleTag.IsValid())
            {
               asc->RemoveLooseGameplayTag(oldReticleTag);
            }

            // Add the new reticle loose tag if there is one
            if (newReticleTag.IsValid())
            {
               asc->AddLooseGameplayTag(newReticleTag);
            }
         }
      }
   }
}

void UTATReticleStateComponent::_MaybeNotifyNewReticleTargetActor(AActor* oldTargetActor)
{
   AActor* newTargetActor = _currentReticleTargetActor;
   if (newTargetActor != oldTargetActor)
   {
      UE_LOG(LogTATReticleStateComponent, Verbose, TEXT("Custom tool reticle target actor changed from '%s' to '%s'"),
         *GetNameSafe(oldTargetActor), *GetNameSafe(newTargetActor));

      OnReticleTargetActorChanged.Broadcast(oldTargetActor, newTargetActor);
   }
}
