// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Graphics/Auras/TATAuraVisibilityPerceiverComponent.h"

// ue
#include "Player/TATPlayerController.h"
#include "GameFramework/PlayerController.h"
#include "Misc/DataValidation.h"

// tat
#include "Graphics/Auras/TATAurasWorldSubsystem.h"
#include "Graphics/Auras/TATAuraVisibilityTargetComponent.h"

// ose
#include "Character/OSECharacterBase.h"

DEFINE_LOG_CATEGORY_STATIC(LogTATAuraVisibilityPerceiver, Log, All);

DECLARE_STATS_GROUP(TEXT("TATAuraSense"), STATGROUP_TATAuraSense, STATCAT_Advanced);

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAuraVisibilityPerceiverComponent)


UTATAuraVisibilityPerceiverComponent::UTATAuraVisibilityPerceiverComponent()
{
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
   bWantsInitializeComponent = true;
}

#if WITH_EDITOR
EDataValidationResult UTATAuraVisibilityPerceiverComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   for (const FTATAuraVisibilityPerceiverSense& auraSense : _auraPerceivedVisibilitySenses)
   {
      if (auraSense.PerceivingPlayerAuraVisibility == ETATAuraVisibilityType::None && auraSense.SharedAuraVisibility == ETATAuraVisibilityType::None)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("%s | _auraPerceivedVisibilitySenses entry %s has a visibility of None for both PerceivingPlayerAuraVisibility and SharedAuraVisibility! Set one of these to a non-None value.")
            , *GetName()
            , *auraSense.AuraSenseTag.ToString())));
         result = EDataValidationResult::Invalid;
      }

      if (!auraSense.AuraSenseTag.IsValid())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("%s | Detected an _auraPerceivedVisibilitySenses entry without a valid AuraSenseTag!"), *GetName())));
         result = EDataValidationResult::Invalid;
      }
   }

   return result;
}
#endif // WITH_EDITOR

void UTATAuraVisibilityPerceiverComponent::InitializeComponent()
{
   Super::InitializeComponent();

   _ownerCharacter = CastChecked<AOSECharacterBase>(GetOwner());
}

void UTATAuraVisibilityPerceiverComponent::BeginPlay()
{
   Super::BeginPlay();

   check(IsValid(_ownerCharacter));
   if (AController* controller = _ownerCharacter->GetController())
   {
      _OnOwnerCharacterPossessed(controller);
   }
   else
   {
      // Bind to OnPossessedBy so we know when the controller has replicated
      _ownerCharacter->OnPossessedBy.AddDynamic(this, &UTATAuraVisibilityPerceiverComponent::_OnOwnerCharacterPossessed);
   }
}

void UTATAuraVisibilityPerceiverComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (ShouldEvaluateAnyAuraSenses())
   {
      _DisableAuraPerception();
   }

   Super::EndPlay(endPlayReason);
}

void UTATAuraVisibilityPerceiverComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   check(ShouldEvaluateAnyAuraSenses());

   DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Tick Aura Sense"), STAT_AuraPerceiverComponent_Tick, STATGROUP_TATAuraSense);

   // Clear any cached target data
   _auraTargetCachedData.Reset();

   // When a client disconnects from the host, there is no guarantee the character owning this component will be destroyed before the controller - it is possible for GetValid() to return false due to pending kill.
   // In this case should early out and wait for EndPlay() to perform necessary teardown.
   if (!IsValid(_ownerPlayerController))
   {
      UE_LOG(LogTATAuraVisibilityPerceiver, Verbose, TEXT("_EvaluateAuraVisibilityPerceiverSense() called for character %s without valid owning controller! \
         (This can happen on client disconnect, but shouldn't be expected in other cases)")
         , *_ownerCharacter->GetName());
      return;
   }

   for (const FTATAuraVisibilityPerceiverSense& auraSense : _auraSensesToEvaluate)
   {
      _EvaluateAuraVisibilityPerceiverSense(auraSense);
   }
}

void UTATAuraVisibilityPerceiverComponent::_EvaluateAuraVisibilityPerceiverSense(const FTATAuraVisibilityPerceiverSense& perceiverSense)
{
   check(IsValid(_ownerCharacter));
   check(IsValid(_ownerPlayerController));

   // We check using ShouldEvaluateAuraSense() (which checks for sense w/ matching tag in _auraSensesToEvaluate)
   // rather than _ShouldEverEvaluateAuraSense() (which checks IsLocallyControlled() / HasAuthority() against IsLocalSense() / IsSharedSense() respectively).
   // The reason for this is IsLocallyControlled() returns false when astral-projecting, and no longer serves as an indication of a local player's pawn instance).
   check(ShouldEvaluateAuraSense(perceiverSense.AuraSenseTag));

   // Check if owner meets various sense requirements
   FGameplayTagContainer ownerCharacterTags;
   _ownerCharacter->GetOwnedGameplayTags(ownerCharacterTags);
   const bool meetsTagRequirements = perceiverSense.PerceiverRequiredTagQuery.IsEmpty() || perceiverSense.PerceiverRequiredTagQuery.Matches(ownerCharacterTags);
   const bool meetsPossessionRequirements = !perceiverSense.RequiresOwnerPossession || _ownerPlayerController->GetPawn() == _ownerCharacter;
   const bool meetsStationaryRequirements = !perceiverSense.RequiresStationaryPerceiver || _ownerPlayerController->GetIsPlayerStandingStill();

   // Pre-capture owner eyes viewpoint/look direction used in LoS checks
   FVector actorEyesViewPoint;
   FRotator actorEyesRotation;
   _ownerCharacter->GetActorEyesViewPoint(actorEyesViewPoint, actorEyesRotation);
   const FVector lookDirection = actorEyesRotation.Quaternion().GetForwardVector();

   // Cache world for LoS-traces
   const UWorld* world = GetWorld();
   check(world);

   // Get aura targets from world subsystem
   UTATAurasWorldSubsystem* aiStateWorldSubsystem = world->GetSubsystem<UTATAurasWorldSubsystem>();
   check(IsValid(aiStateWorldSubsystem));
   const TArray<TWeakObjectPtr<UTATAuraVisibilityTargetComponent>>& auraVisibilityTargets = aiStateWorldSubsystem->GetAuraVisibilityTargets();
   for (const TWeakObjectPtr<UTATAuraVisibilityTargetComponent> auraVisibilityTargetWeakPtr : auraVisibilityTargets)
   {
      UTATAuraVisibilityTargetComponent* auraVisibilityTarget = auraVisibilityTargetWeakPtr.Get();
      check(IsValid(auraVisibilityTarget));
      check(IsValid(auraVisibilityTarget->GetOwner()));

      if(!meetsTagRequirements || !meetsPossessionRequirements || !meetsStationaryRequirements)
      {
         auraVisibilityTarget->UpdateAuraPerceiverSenseEntry(MakeWeakObjectPtr(this), perceiverSense.AuraSenseTag, false);
         continue;
      }

      // Trim actors out of effective range
      const float distance = FVector::Distance(_ownerCharacter->GetActorLocation(), auraVisibilityTarget->GetOwner()->GetActorLocation());
      if (distance > perceiverSense.EffectiveRange)
      {
         auraVisibilityTarget->UpdateAuraPerceiverSenseEntry(MakeWeakObjectPtr(this), perceiverSense.AuraSenseTag, false);
         continue;
      }

      if (perceiverSense.RequiresLineOfSight)
      {
         bool hasLineOfSight = false;

         // Retrieve cached line-of-sight entry if previously computed for target
         const FTATAuraVisibilityTargetCachedData* cachedData = _auraTargetCachedData.FindByPredicate([&](const FTATAuraVisibilityTargetCachedData& cachedData)
         { return cachedData.AuraTarget == auraVisibilityTarget; });
         if (cachedData)
         {
            hasLineOfSight = cachedData->PerceiverHasLineOfSight;
         }
         else
         {
            // Trim actors outside player's camera to skip unnecessary LoS traces
            const FVector eyesToTarget = auraVisibilityTarget->GetOwner()->GetActorLocation() - actorEyesViewPoint;
            const float lookAtTargetDot = FVector::DotProduct(eyesToTarget.GetSafeNormal(), lookDirection.GetSafeNormal());
            if (lookAtTargetDot <= perceiverSense.LineOfSightDotProductThreshold)
            {
               auraVisibilityTarget->UpdateAuraPerceiverSenseEntry(MakeWeakObjectPtr(this), perceiverSense.AuraSenseTag, false);
               continue;
            }

            {
               DECLARE_SCOPE_CYCLE_COUNTER(TEXT("Line Of Sight Check"), STAT_AuraPerceiverComponent_LineOfSight, STATGROUP_TATAuraSense);

               // If target is within camera frustum, perform LoS trace
               FVector traceStart = actorEyesViewPoint;
               FVector traceEnd = auraVisibilityTarget->GetOwner()->GetActorLocation();

               // Ignore self + target
               FCollisionQueryParams queryParams(SCENE_QUERY_STAT(AuraPerceiverTrace));
               queryParams.AddIgnoredActor(_ownerCharacter);
               queryParams.AddIgnoredActor(auraVisibilityTarget->GetOwner());

               hasLineOfSight = !world->LineTraceTestByChannel(traceStart, traceEnd, _auraLineOfSightCollisionChannel, queryParams);

               // Cache line-of-sight data for this target so we don't have to re-compute for other senses
               _auraTargetCachedData.Emplace(auraVisibilityTargetWeakPtr, hasLineOfSight);
            }
         }
         
         auraVisibilityTarget->UpdateAuraPerceiverSenseEntry(MakeWeakObjectPtr(this), perceiverSense.AuraSenseTag, hasLineOfSight);
      }
      else
      {
         auraVisibilityTarget->UpdateAuraPerceiverSenseEntry(MakeWeakObjectPtr(this), perceiverSense.AuraSenseTag, true);
      }
   }
}

void UTATAuraVisibilityPerceiverComponent::_EnableAuraPerception()
{
   check(IsValid(_ownerCharacter));
   check(ShouldEvaluateAnyAuraSenses());

   UE_LOG(LogTATAuraVisibilityPerceiver, Verbose, TEXT("%s - enabling aura perception..."), *_ownerCharacter->GetName());

   // Register self with world subsystem so targets can register themselves w/ us
   if (const UWorld* world = GetWorld())
   {
      UTATAurasWorldSubsystem* aiStateWorldSubsystem = world->GetSubsystem<UTATAurasWorldSubsystem>();
      check(IsValid(aiStateWorldSubsystem));
      aiStateWorldSubsystem->RegisterAuraVisibilityPerceiver(this);
   }
   else
   {
      UE_LOG(LogTATAuraVisibilityPerceiver, Warning, TEXT("%s | _EnableAuraPerception() failed to register aura perceiver (GetWorld() returned nullptr)"), *_ownerCharacter->GetName());
   }

   // Tick to track target perception states
   PrimaryComponentTick.SetTickFunctionEnable(true);
}

void UTATAuraVisibilityPerceiverComponent::_DisableAuraPerception()
{
   check(IsValid(_ownerCharacter));
   check(ShouldEvaluateAnyAuraSenses());

   UE_LOG(LogTATAuraVisibilityPerceiver, Verbose, TEXT("%s - disabling aura perception..."), *_ownerCharacter->GetName());

   // Unregister self with world subsystem so targets can unregister themselves w/ us + clear related state
   if (const UWorld* world = GetWorld())
   {
      if (UTATAurasWorldSubsystem* aiStateWorldSubsystem = world->GetSubsystem<UTATAurasWorldSubsystem>())
      {
         aiStateWorldSubsystem->UnregisterAuraVisibilityPerceiver(this);
      }
   }
   else
   {
      UE_LOG(LogTATAuraVisibilityPerceiver, Warning, TEXT("%s | _DisableAuraPerception() failed to unregister aura perceiver (GetWorld() returned nullptr)"), *_ownerCharacter->GetName());
   }

   PrimaryComponentTick.SetTickFunctionEnable(false);
}

void UTATAuraVisibilityPerceiverComponent::_OnOwnerCharacterPossessed(AController* controller)
{
   check(IsValid(controller));
   check(IsValid(_ownerCharacter));

   // OnPossessedBy has served its purpose of notifying us of a replicated controller + owner possession, so we can unbind now
   _ownerCharacter->OnPossessedBy.RemoveAll(this);

   // Enable aura perception if there's any senses that need evaluation on this machine
   _auraSensesToEvaluate = _GetAuraSensesToEvaluate();
   if (_auraSensesToEvaluate.Num() > 0)
   {
      _ownerPlayerController = CastChecked<ATATPlayerController>(controller);
      _EnableAuraPerception();
   }
}

TArray<FTATAuraVisibilityPerceiverSense> UTATAuraVisibilityPerceiverComponent::_GetAuraSensesToEvaluate() const
{
   check(IsValid(_ownerCharacter));

   TArray<FTATAuraVisibilityPerceiverSense> sensesToEvaluate;
   for (const FTATAuraVisibilityPerceiverSense& auraSense : _auraPerceivedVisibilitySenses)
   {
      if (_ShouldEverEvaluateAuraSense(auraSense))
      {
         sensesToEvaluate.Add(auraSense);
      }
   }

   return sensesToEvaluate;
}

bool UTATAuraVisibilityPerceiverComponent::_ShouldEverEvaluateAuraSense(const FTATAuraVisibilityPerceiverSense& auraSense) const
{
   check(IsValid(_ownerCharacter));
   if (auraSense.IsSharedSense())
   {
      return _ownerCharacter->HasAuthority();
   }

   if (auraSense.IsLocalSense())
   {
      return _ownerCharacter->IsLocallyControlled();
   }

   UE_LOG(LogTATAuraVisibilityPerceiver, Warning, TEXT("_ShouldEverEvaluateAuraSense() | aura sense %s on perceiver %s is neither local nor shared!")
      , *GetOwner()->GetName()
      , *auraSense.AuraSenseTag.ToString());
   return false;
}
