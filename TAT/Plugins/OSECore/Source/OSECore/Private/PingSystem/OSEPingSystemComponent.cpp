// (c) 2021-2021 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "PingSystem/OSEPingSystemComponent.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "PingSystem/OSEPingActor.h"
#include "Player/OSEPlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPingSystemComponent)

// ue4
UOSEPingSystemComponent::UOSEPingSystemComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
}

void UOSEPingSystemComponent::BeginPlay()
{
   Super::BeginPlay();

   if (_IsLocallyControlled())
   {
      SetComponentTickEnabled(true);
   }
}

void UOSEPingSystemComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);
   _TickLocalFocusedPingActor();
}

void UOSEPingSystemComponent::ServerSpawnPing_Implementation(const FOSEPingSpawnInfo& spawnInfo, const FHitResult& hitResult, const FTransform& spawnerViewXfm, const FTransform& spawnerWorldXfm, const FGameplayTag& pingTag, AActor* pingableTargetActor)
{
   // assuming reasonable usage of the system instead of just nullchecking/erroring all over the place here
   check(PingInfoAsset && PingInfoAsset->DefaultPingActorClass);

   // require ping info to spawn it
   if (const FOSEPingInfo* pingInfo = PingInfoAsset->FindPingInfoFromPingTag(pingTag))
   {
      TSubclassOf<AOSEPingActor> pingClassToSpawn = pingInfo->OverridePingActorClass ? pingInfo->OverridePingActorClass : PingInfoAsset->DefaultPingActorClass;
      check(pingClassToSpawn);
      
      FActorSpawnParameters spawnParams;
      spawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
      spawnParams.ObjectFlags |= RF_Transient;
      spawnParams.bDeferConstruction = true;

      FVector worldLocation = FVector::ZeroVector;
      USceneComponent* attachTo = nullptr;
      FName attachSocket;
      switch(spawnInfo.SpawnType)
      {
      case EOSEPingSpawnType::AtPingWorldLocation:
         {
            worldLocation = hitResult.Location;
         }
         break;
      case EOSEPingSpawnType::AtSpecificWorldLocation:
         {
            worldLocation = spawnInfo.Location;
         }
         break;
      case EOSEPingSpawnType::AttachToActor:
         {
            attachTo = spawnInfo.AttachComponent;
            attachSocket = spawnInfo.AttachSocket;
         }
         break;
      }

      FRotator zeroRotation = FRotator::ZeroRotator; // rot doesn't matter for a ping, it's a world location for a HUD item
      AOSEPingActor* pingActor = CastChecked<AOSEPingActor>(GetWorld()->SpawnActor(pingClassToSpawn, &worldLocation, &zeroRotation, spawnParams));
      if (attachTo)
      {
         pingActor->AttachToComponent(attachTo, FAttachmentTransformRules::KeepRelativeTransform, attachSocket);
      }
      pingActor->SetInstigator(_GetOwnerController().GetPawn());
      pingActor->AuthoritySetup(this, pingTag, pingableTargetActor, hitResult, spawnerViewXfm, spawnerWorldXfm);
      pingActor->FinishSpawning(FTransform(zeroRotation, worldLocation, FVector(1.0f, 1.0f, 1.0f)));

      // blueprints can handle this and do something like pipe the info into the event feed
      OnAuthoritySpawnedPing.Broadcast(pingActor);
   }
   else
   {
      UE_LOG(LogOSEPingSystem, Error, TEXT("Ping tag %s not found in ping info asset!"), *pingTag.ToString());
   }
}

bool UOSEPingSystemComponent::ServerSpawnPing_Validate(const FOSEPingSpawnInfo& spawnInfo, const FHitResult& hitResult, const FTransform& spawnerViewXfm, const FTransform& spawnerWorldXfm, const FGameplayTag& pingTag, AActor* pingableTargetActor)
{
   // trusting the client, this is a non-gameplay action
   return true;
}

void UOSEPingSystemComponent::ServerCancelPing_Implementation(AOSEPingActor* pingActor, bool isPlayerInitiated)
{
   if (!IsValid(pingActor))
   {
      // we're canceling a ping that died by itself while this message was in-flight, so just ignore it?
      // TODO: We could send up the tag instead if we want a different behavior here...?
      return;
   }

   FGameplayTag pingTag = pingActor->GetPingTag();

   // destroy to cancel!  this triggers the actor's EndPlay() which removes it from bookkeeping
   pingActor->Destroy();

   // broadcast
   OnAuthorityCanceledPing.Broadcast(pingTag, isPlayerInitiated);
}

bool UOSEPingSystemComponent::ServerCancelPing_Validate(AOSEPingActor* pingActor, bool isPlayerInitiated)
{
   // trusting the client, this is a non-gameplay action
   return true;
}

void UOSEPingSystemComponent::ServerRespondToPing_Implementation(AOSEPingActor* pingActor, const FGameplayTag& responseTag)
{
   if (!IsValid(pingActor))
   {
      // we're responding to a ping that died while this message was in-flight, so just ignore it?
      // TODO: We could send up the tag instead if we want a different behavior here...?
      return;
   }

   // blueprints can handle this and do something like pipe the info into the event feed
   OnAuthorityRespondedToPing.Broadcast(pingActor, responseTag);
}

bool UOSEPingSystemComponent::ServerRespondToPing_Validate(AOSEPingActor* pingActor, const FGameplayTag& responseTag)
{
   // trusting the client, this is a non-gameplay action
   return true;
}

void UOSEPingSystemComponent::AuthorityAddSpawnedPingActor(AOSEPingActor* pingActor)
{
   check(GetOwner()->HasAuthority());
   _AuthorityAddPingToBookkeeping(pingActor);
}

void UOSEPingSystemComponent::AuthorityRemoveSpawnedPingActor(AOSEPingActor* pingActor)
{
   check(GetOwner()->HasAuthority());
   _AuthorityRemovePingFromBookkeeping(pingActor);
}

void UOSEPingSystemComponent::LocallyAddSpawnedPingActor(AOSEPingActor* pingActor)
{
   check(pingActor);
   check(_IsLocallyControlled());
   _localPingActors.Add(pingActor);
}

void UOSEPingSystemComponent::LocallyRemoveSpawnedPingActor(AOSEPingActor* pingActor)
{
   check(pingActor);
   check(_IsLocallyControlled());
   _localPingActors.Remove(pingActor);

   if (_localFocusPingActor.IsValid())
   {
      _localFocusPingActor = nullptr;
      OnLocalFocusedPingActorChanged.Broadcast(_localFocusPingActor.Get());
   }
}

APlayerController& UOSEPingSystemComponent::_GetOwnerController() const
{
   return *CastChecked<APlayerController>(GetOwner());
}

bool UOSEPingSystemComponent::_IsLocallyControlled() const
{
   return _GetOwnerController().IsLocalController();
}

void UOSEPingSystemComponent::_AuthorityAddPingToBookkeeping(AOSEPingActor* pingActor)
{
   check(pingActor);

   const FGameplayTag& pingTag = pingActor->GetPingTag();
   check(pingTag.IsValid());
   const FOSEPingInfo* pingInfo = PingInfoAsset->FindPingInfoFromPingTag(pingTag);
   check(pingInfo);

   // add to back
   _serverSpawnedPingActors.FindOrAdd(pingTag).Add(pingActor);

   // clean up any old ones based on how many of these we're allowed to have out, starting from the front
   TArray<TWeakObjectPtr<AOSEPingActor>>& spawnedPings = _serverSpawnedPingActors[pingTag];
   while (spawnedPings.Num() > 0 && spawnedPings.Num() > pingInfo->NumSimultaneousAllowed)
   {
      if(AOSEPingActor* pingToKill = spawnedPings[0].Get())
      {
         ServerCancelPing(pingToKill, false);
      }
   }
}

void UOSEPingSystemComponent::_AuthorityRemovePingFromBookkeeping(AOSEPingActor* pingActor)
{
   check(pingActor);

   const FGameplayTag& pingTag = pingActor->GetPingTag();
   check(pingTag.IsValid());

   TArray<TWeakObjectPtr<AOSEPingActor>>& spawnedPings = _serverSpawnedPingActors[pingTag];
   if (spawnedPings.Contains(pingActor))
   {
      spawnedPings.Remove(pingActor);
   }
   else
   {
      UE_LOG(LogOSEPingSystem, Error, TEXT("Ping actor with tag %s was not found in our bookkeeping?!"), *pingTag.ToString());
   }
}

void UOSEPingSystemComponent::_TickLocalFocusedPingActor()
{
   QUICK_SCOPE_CYCLE_COUNTER(STAT_UOSEPingSystemComponent_TickLocalFocusedPingActor);

   check(_IsLocallyControlled());

   const AOSEPlayerController& localPC = *CastChecked<AOSEPlayerController>(GetOwner());

   // the focused ping actor is the ping that's closest to the center of the screen
   AOSEPingActor* focusedPingActor = nullptr;
   float focusedPingActorDistanceFromCenter = static_cast<float>(INDEX_NONE);

   int32 sizeX, sizeY;
   localPC.GetViewportSize(sizeX, sizeY);
   
   const FVector2D viewportSize = FVector2D(sizeX, sizeY);
   const FVector2D viewportCenter = viewportSize / 2.0f;
   const FVector2D viewportSizeScaled = viewportSize * FocusRadiusPercent;
   const float radius = (viewportSizeScaled.X < viewportSizeScaled.Y ? viewportSizeScaled.X : viewportSizeScaled.Y) / 2.0f;

   for(TWeakObjectPtr<AOSEPingActor> pingActor : _localPingActors)
   {
      // shouldn't have any holes but... for sanity.
      if (!pingActor.IsValid())
         continue;

      // ignore pings that don't want to be focused
      const FGameplayTag& pingTag = pingActor->GetPingTag();
      const FOSEPingInfo* pingInfo = PingInfoAsset->FindPingInfoFromPingTag(pingTag);
      if (pingInfo && !pingInfo->AllowFocus)
         continue;

      const USceneComponent* pingUIComponent = pingActor->GetPingUILocation();
      check(pingUIComponent);
      const FVector worldLocation = pingUIComponent->GetComponentLocation();

      FVector2D pingScreenLocation;
      if (localPC.ProjectWorldLocationToScreen(worldLocation, pingScreenLocation))
      {
         // is this the closest ping to the center of the screen?
         // and is it close enough to the center of the screen to focus it?
         const bool isPointInside = FVector2D::DistSquared(viewportCenter, pingScreenLocation) < (radius * radius);
         const bool isReady = pingActor->IsLocallyReadyToBeFocused();

         float distanceFromCenter = FMath::Abs(FVector2D::Distance(viewportCenter, pingScreenLocation));
         if (isPointInside && isReady &&
             (focusedPingActorDistanceFromCenter == float(INDEX_NONE) || distanceFromCenter < focusedPingActorDistanceFromCenter))
         {
            focusedPingActorDistanceFromCenter = distanceFromCenter;
            focusedPingActor = pingActor.Get();
         }
      }
   }

   if (focusedPingActor != _localFocusPingActor)
   {
      _localFocusPingActor = focusedPingActor;
      OnLocalFocusedPingActorChanged.Broadcast(_localFocusPingActor.Get());
   }
}

