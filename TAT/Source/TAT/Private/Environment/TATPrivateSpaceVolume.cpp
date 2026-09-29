// (c) 2022-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Environment/TATPrivateSpaceVolume.h"

// tat
#include "AI/Environment/TATAIContextualLocation.h"
#include "Disguise/TATDisguiseComponent.h"
#include "Environment/TATPrivateSpaceCharacterComponent.h"
#include "Environment/TATPrivateSpaceCharacterInterface.h"
#include "Variation/SceneVariants/TATSceneVariantUtils.h"
#include "Variation/TATMapVariationMgrComponent.h"
#include "Online/TATGameState.h"
#include "Player/TATCharacter.h"
#include "AI/Squad/TATSquadAlarmStation.h"

// ue
#include "Components/BrushComponent.h"
#include "NativeGameplayTags.h"
#include "UObject/ConstructorHelpers.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPrivateSpaceVolume)

DEFINE_LOG_CATEGORY_STATIC(LogTATPrivateSpace, Log, All);
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_PRIVATESPACE_DEFAULT, "PrivateSpace.Default");

ATATPrivateSpaceVolume::ATATPrivateSpaceVolume(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer)
{
   PrimaryActorTick.bCanEverTick = false;
   PrimaryActorTick.bStartWithTickEnabled = false;
   bNetLoadOnClient = false;

   _privateZoneGameplayTag = TAG_PRIVATESPACE_DEFAULT;
   
   static ConstructorHelpers::FClassFinder<UNavArea> navAreaClass(TEXT("/Game/AI/Navigation/NavArea_PrivateArea"));
   if (navAreaClass.Class != nullptr)
      AreaClass = navAreaClass.Class;
   
   if(UBrushComponent* brush = GetBrushComponent())
   {
      static FName collisionProfileName(TEXT("Trigger"));
      brush->SetCollisionProfileName(collisionProfileName);
      brush->SetGenerateOverlapEvents(true);
   }
}

bool IsMapVariationLoadingStateIsValid(const ETATMapVariationLoadingState state)
{
   return state == ETATMapVariationLoadingState::CompleteNoVariation || state == ETATMapVariationLoadingState::CompleteWithVariation;
}

void ATATPrivateSpaceVolume::_HandleMapStateChanged(const ETATMapVariationLoadingState currentState)
{
   if(IsMapVariationLoadingStateIsValid(currentState))
   {
      _WaitForWorldBegunPlayOrTrigger();
   }
}

void ATATPrivateSpaceVolume::_WaitForWorldBegunPlayOrTrigger()
{
   if(UWorld* world = GetWorld())
   {
      if(world->HasBegunPlay())
      {
         _OnWorldBegunPlay();
      }
      else
      {
         world->OnWorldBeginPlay.AddUObject(this, &ThisClass::_OnWorldBegunPlay);
      }
   }
}

void ATATPrivateSpaceVolume::BeginPlay()
{
   _isReadyToProcessActorOverlaps = false;
   Super::BeginPlay();
   if (HasAuthority() == false)
   {
      UE_LOG(LogTATPrivateSpace, Warning, TEXT("Private space volume '%s' is running on client, though it should be authority-only"), *GetName());
      return;
   }

   if (_overrideScene)
   {
      _spaceType = UTATSceneVariantUtils::ResolvePrivacyForScene(GetWorld(), _overrideScene, _spaceType);
   }

   // If it is a public space, clear the nav area, but keep around mostly-disabled
   // Just to mark newly-spawned actors with it's tag. (Is that worth it?)
   if (_spaceType == ETATPrivateSpaceType::PublicArea)
   {
      SetAreaClass(nullptr);
   }

   bool bShouldBindOnWorldBeginPlay = true;
   if (const ATATGameState* gameState = Cast<ATATGameState>(GetWorld()->GetGameState()))
   {
      if(UTATMapVariationMgrComponent* mapVariationMgr = gameState->GetMapVariationMgr())
      {
         const ETATMapVariationLoadingState state = mapVariationMgr->GetCurrentMapVariationLoadingState();
         if(IsMapVariationLoadingStateIsValid(state) == false)
         {
            mapVariationMgr->OnMapVariationMgrStateChanged.AddDynamic(this, &ThisClass::_HandleMapStateChanged);
         }
         else
         {
            _HandleMapStateChanged(state);            
         }
         bShouldBindOnWorldBeginPlay = false;
      }
   }
   
   if(bShouldBindOnWorldBeginPlay)
   {
      _WaitForWorldBegunPlayOrTrigger();
   }
}

void ATATPrivateSpaceVolume::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   // For any characters still in the volume, let them know they're no longer in it since we're going away
   // This happens after we unsubscribe from the callback, so we won't double-count any exits
   for (auto iterator = _actorsWhoEnteredIntoVolume.CreateIterator(); iterator; ++iterator )
   {
      TScriptInterface<ITATPrivateSpaceCharacterInterface> character = *iterator;
      if (ITATPrivateSpaceCharacterInterface* characterPtr = character.GetInterface())
      {
         if(UTATPrivateSpaceCharacterComponent* privateSpaceCharacterComponent = characterPtr->GetPrivateSpaceCharacterComponent())
         {
            _RemoveSpacePrivacySettings(privateSpaceCharacterComponent);
         }
      }
   }
   Super::EndPlay(endPlayReason);
}

void ATATPrivateSpaceVolume::_OnWorldBegunPlay()
{
   // This was previously set to add the privacy settings for the character when they are starting within a zone, however with the new design,
   // we want the characters starting within a volume to be "allowed" in that volume and never turned suspicious
   if(UWorld* world = GetWorld())
   {
      world->OnWorldBeginPlay.RemoveAll(this);
   }
   HandleInitialOverlaps();
}

void ATATPrivateSpaceVolume::HandleInitialOverlaps()
{
   TArray<AActor*> overlappingActors;
   GetOverlappingActors(overlappingActors, AOSECharacterBase::StaticClass());
   
   _isReadyToProcessActorOverlaps = true;
   
   for (AActor* const overlappingActor : overlappingActors)
   {
      if (overlappingActor && overlappingActor->Implements<UTATPrivateSpaceCharacterInterface>())
      {
         if(ITATPrivateSpaceCharacterInterface* privateSpaceCharacterInterface = Cast<ITATPrivateSpaceCharacterInterface>(overlappingActor))
         {
            if(privateSpaceCharacterInterface->CanEverBeAllowedInPrivateArea())
            {
               if(_privateZoneGameplayTag.IsValid())
               {
                  privateSpaceCharacterInterface->GetPrivateSpaceCharacterComponent()->AuthorityAddAllowedPrivateZone(_privateZoneGameplayTag);
               }
            }
            else
            {
               _StartTrackActor(overlappingActor);
               _AddActorToVolume(overlappingActor);
            }
         }
      }
   }

   overlappingActors.Empty();
   GetOverlappingActors(overlappingActors, ATATAIContextualLocation::StaticClass());
   for (AActor* const overlappingActor : overlappingActors)
   {
      if(ATATAIContextualLocation* contextualLocation = Cast<ATATAIContextualLocation>(overlappingActor))
      {
         contextualLocation->AuthoritySetPrivateZoneTag(_privateZoneGameplayTag);
      }
   }
   
   overlappingActors.Empty();
   GetOverlappingActors(overlappingActors, ATATSquadAlarmStation::StaticClass());
   for (AActor* const overlappingActor : overlappingActors)
   {
      if(ATATSquadAlarmStation* alarmStation = Cast<ATATSquadAlarmStation>(overlappingActor))
      {
         alarmStation->AuthoritySetPrivateZoneTag(_privateZoneGameplayTag);
      }
   }   
}

void ATATPrivateSpaceVolume::_StartTrackActor(AActor* actor)
{
   if(actor && actor->Implements<UTATDisguisableCharacterInterface>())
   {
      if(UTATDisguiseComponent* disguiseComponent = ITATDisguisableCharacterInterface::Execute_GetDisguiseComponent(actor))
      {
         disguiseComponent->OnDisguiseNativeBegin.AddUObject(this, &ThisClass::HandlePrivateSpaceActorDisguiseBegin, actor);
         disguiseComponent->OnDisguiseNativeEnd.AddUObject(this, &ThisClass::HandlePrivateSpaceActorDisguiseEnd, actor);
      }
   }
   if(ITATPrivateSpaceCharacterInterface* privateSpaceCharacterInterface = Cast<ITATPrivateSpaceCharacterInterface>(actor))
   {
      if(UTATPrivateSpaceCharacterComponent* privateSpaceComponent = privateSpaceCharacterInterface->GetPrivateSpaceCharacterComponent())
      {
         privateSpaceComponent->OnTemporaryAllowedChanged.AddUObject(this, &ThisClass::_HandleTemporaryAllowedSpaceChanged);
      }
   }
}

void ATATPrivateSpaceVolume::_EndTrackActor(AActor* actor)
{
   if(actor && actor->Implements<UTATDisguisableCharacterInterface>())
   {
      if(UTATDisguiseComponent* disguiseComponent = ITATDisguisableCharacterInterface::Execute_GetDisguiseComponent(actor))
      {
         disguiseComponent->OnDisguiseNativeBegin.RemoveAll(this);
         disguiseComponent->OnDisguiseNativeEnd.RemoveAll(this);
      }
   }

   if(ITATPrivateSpaceCharacterInterface* privateSpaceCharacterInterface = Cast<ITATPrivateSpaceCharacterInterface>(actor))
   {
      if(UTATPrivateSpaceCharacterComponent* privateSpaceComponent = privateSpaceCharacterInterface->GetPrivateSpaceCharacterComponent())
      {
         privateSpaceComponent->OnTemporaryAllowedChanged.RemoveAll(this);
      }
   }
}

void ATATPrivateSpaceVolume::NotifyActorBeginOverlap(AActor* otherActor)
{
   Super::NotifyActorBeginOverlap(otherActor);
   if (otherActor && otherActor->Implements<UTATPrivateSpaceCharacterInterface>())
   {
      _AddActorToVolume(otherActor);
      _StartTrackActor(otherActor);
   }
}

void ATATPrivateSpaceVolume::NotifyActorEndOverlap(AActor* otherActor)
{
   Super::NotifyActorEndOverlap(otherActor);
   if (otherActor && otherActor->Implements<UTATPrivateSpaceCharacterInterface>())
   {
      _RemoveActorFromVolume(otherActor);
      _EndTrackActor(otherActor);
   }
}

void ATATPrivateSpaceVolume::HandleActor(const TScriptInterface<ITATPrivateSpaceCharacterInterface>& characterInterface)
{
   // public areas are not tracked
   if (GetSpaceType() == ETATPrivateSpaceType::PublicArea)
      return;
   if(_ShouldAddActorToVolume(characterInterface))
   {
      if(_actorsWhoEnteredIntoVolume.Contains(characterInterface) == false)
      {
         _AddActorToVolume(characterInterface);
      }
   }
   else
   {
      if(_actorsWhoEnteredIntoVolume.Contains(characterInterface))
      {
         _RemoveActorFromVolume(characterInterface);
      }
   }
}

void ATATPrivateSpaceVolume::HandlePrivateSpaceActorDisguiseBegin(AActor* tatCharacter)
{
   HandleActor(tatCharacter);
}

void ATATPrivateSpaceVolume::HandlePrivateSpaceActorDisguiseEnd(AActor* tatCharacter)
{
   HandleActor(tatCharacter);
}

void ATATPrivateSpaceVolume::_HandleTemporaryAllowedSpaceChanged(FGameplayTag spaceTag, AActor* actor)
{
   if(spaceTag.MatchesTag(_privateZoneGameplayTag))
   {
      HandleActor(actor);
   }
}

void ATATPrivateSpaceVolume::_AddActorToVolume(const TScriptInterface<ITATPrivateSpaceCharacterInterface>& privateSpaceCharacterInterface)
{
   if(_ShouldAddActorToVolume(privateSpaceCharacterInterface) == false)
      return;
   
   UTATPrivateSpaceCharacterComponent* privateSpaceComponent = privateSpaceCharacterInterface->GetPrivateSpaceCharacterComponent();
   check(privateSpaceComponent);
   
   check(_actorsWhoEnteredIntoVolume.Contains(privateSpaceCharacterInterface) == false);
   _actorsWhoEnteredIntoVolume.Add(privateSpaceCharacterInterface);
   _ApplySpacePrivacySettings(privateSpaceComponent);

}

void ATATPrivateSpaceVolume::_RemoveActorFromVolume(const TScriptInterface<ITATPrivateSpaceCharacterInterface>& privateSpaceCharacterInterface)
{
   if(_ShouldAddActorToVolume(privateSpaceCharacterInterface) == false &&
      _actorsWhoEnteredIntoVolume.Contains(privateSpaceCharacterInterface) == false)
      return;
   
   UTATPrivateSpaceCharacterComponent* privateSpaceComponent = privateSpaceCharacterInterface->GetPrivateSpaceCharacterComponent();
   check(privateSpaceComponent);
   
   check(_actorsWhoEnteredIntoVolume.Contains(privateSpaceCharacterInterface));
   _actorsWhoEnteredIntoVolume.Remove(privateSpaceCharacterInterface);
   _RemoveSpacePrivacySettings(privateSpaceComponent);

}

void ATATPrivateSpaceVolume::_ApplySpacePrivacySettings(UTATPrivateSpaceCharacterComponent* privateSpaceCharacterComponent)
{
   // If this enum grows, I'll need to rethink this pattern.
   check(GetSpaceType() != ETATPrivateSpaceType::PublicArea);
   privateSpaceCharacterComponent->AuthorityOnEnterPrivateSpaceVolume(GetSpaceType() == ETATPrivateSpaceType::OffLimits);
}

void ATATPrivateSpaceVolume::_RemoveSpacePrivacySettings(UTATPrivateSpaceCharacterComponent* privateSpaceCharacterComponent)
{
   check(GetSpaceType() != ETATPrivateSpaceType::PublicArea);
   privateSpaceCharacterComponent->AuthorityOnExitPrivateSpaceVolume(GetSpaceType() == ETATPrivateSpaceType::OffLimits);
}

bool ATATPrivateSpaceVolume::_ShouldAddActorToVolume(const TScriptInterface<ITATPrivateSpaceCharacterInterface>& privateSpaceCharacterInterface) const
{
   if(_isReadyToProcessActorOverlaps == false)
       return false;

   // public areas are not tracked
   if (GetSpaceType() == ETATPrivateSpaceType::PublicArea)
   {
      return false;
   }
   
   if(privateSpaceCharacterInterface == nullptr)
      return false;
   
   // 1. If the character didn't start in the volume BUT has been given permission to be in the private zone via a tag,
   // allow them to be there (return false)
   if(_privateZoneGameplayTag.IsValid())
   {
      if(privateSpaceCharacterInterface->GetPrivateSpaceCharacterComponent()->AuthorityIsAllowedInPrivateZone(_privateZoneGameplayTag))
         return false;
   }
   // 2. if the target CAN become suspicious or intruder, then add them to the volume.
   return privateSpaceCharacterInterface->CanBecomeSuspiciousOrIntruder();
}
