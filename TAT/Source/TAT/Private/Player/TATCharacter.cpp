// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Player/TATCharacter.h"

// tat
#include "Abilities/TATStaminaAttributeSet.h"
#include "Abilities/TATGameplayTags.h"
#include "AI/TATAIController.h"
#include "AI/TATAISettings.h"
#include "AI/TATAIStateWorldSubsystem.h"
#include "Animation/TATCharacterAnimationMapping.h"
#include "Camera/TATFirstPersonViewModifier.h"
#include "Character/TATCharacterMovement.h"
#include "CharacterCustomization/TATCharacterOutfits.h"
#include "Collision/TATCollisionUtils.h"
#include "Collision/Overlay/CollisionOverlayCapsuleComponent.h"
#include "Combat/TATCombatComponent.h"
#include "Combat/TATCombatFunctionLibrary.h"
#include "Combat/TATCombatSettings.h"
#include "Developer/TATOutfitSettings.h"
#include "Developer/TATProjectSettings.h"
#include "Disguise/TATDisguiseComponent.h"
#include "Indicators/TATThiefVisionSubsystem.h"
#include "Interactables/TATInteractionTargeterComponent.h"
#include "Items/TATItemFunctionLibrary.h"
#include "Items/TATItemInventoryComponent.h"
#include "Loot/TATLootInventory.h"
#include "Online/TATGameState.h"
#include "Online/TATPvPGameMode.h"
#include "Player/TATPlayerState.h"
#include "Tools/TATToolSetComponent.h"
#include "UI/TATToastBroadcaster.h"
#include "Environment/TATPrivateSpaceCharacterComponent.h"
#include "Environment/TATPrivateSpaceGameplayTagDefines.h"
#include "Character/TATTeams.h"
#include "AI/Detection/TATLightDetectionCharacterComponent.h"
#include "Character/TATKnockoutHandlerInterface.h"
#include "AI/UnifiedStealthSystem/TATStealthScoreComponent.h"
#include "Settings/TATMatchSettings.h"
#include "WorldMap/TATWorldMapSubsystem.h"
#include "Character/TATTeamsSubsystem.h"
#include "Graphics/TATXrayComponent.h"
#include "Player/TATPlayerController.h"
#include "Analytics/TATAnalyticsManager.h"
#include "Settings/TATGameUserSettings.h"

// ose
#include "OSECommon.h"
#include "OSECoreCollision.h"
#include "Abilities/OSEAbilityFunctionLibrary.h"
#include "Abilities/OSEAbilitySystemComponent.h"
#include "AI/OSEAIFunctionLibrary.h"
#include "AI/Utility/UtilityAITokenOwnerGameplayTagCount.h"
#include "Input/OSEInputSettings.h"
#include "Items/ToolComponent.h"
#include "Items/ToolSetComponent.h"
#include "Player/OSEPlayerController.h"
#include "Player/OSEPlayerState.h"

// ue4
#include "AbilitySystemGlobals.h"
#include "AkGameplayStatics.h"
#include "AkRtpc.h"
#include "AkSwitchValue.h"
#include "Animation/AnimInstance.h"
#include "Components/CapsuleComponent.h"
#include "DrawDebugHelpers.h"
#include "Camera/CameraComponent.h"
#include "Engine/AssetManager.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/DataValidation.h"
#include "Net/UnrealNetwork.h"
#include "PhysicalMaterials/PhysicalMaterial.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATCharacter)

namespace CharacterCVars
{
   static int32 DebugDrawAILineOfSightLocations = 0;
   FAutoConsoleVariableRef CVarDebugDrawAILineOfSightLocations(
      TEXT("tat.Player.DebugDrawAILineOfSightLocations"),
      DebugDrawAILineOfSightLocations,
      TEXT("Should we debug draw the locations the AI is checking to spot the player?"),
      ECVF_Default);

   static int32 DebugDrawAILineOfSightChecks = 0;
   FAutoConsoleVariableRef CVarDebugDrawAILineOfSightChecks(
      TEXT("tat.Player.DebugDrawAILineOfSightChecks"),
      DebugDrawAILineOfSightChecks,
      TEXT("Should we debug draw the checks done by the AI when trying to spot the player?"),
      ECVF_Default);
   
   static int32 DebugDrawAILightDetectionChecks = 0;
   FAutoConsoleVariableRef CVarDebugDrawAILightDetectionChecks(
      TEXT("tat.Player.DebugDrawAILightDetectionChecks"),
      DebugDrawAILightDetectionChecks,
      TEXT("Should we debug draw the checks done for light detection?"),
      ECVF_Default);

   static bool ReducedCameraBounce = 0;
   FAutoConsoleVariableRef CVarReducedCameraBounce(
      TEXT("tat.Player.ReducedCameraBounce"),
      ReducedCameraBounce,
      TEXT("Should we disable a bunch of camera bouncing"),
      ECVF_Default);
}

DEFINE_LOG_CATEGORY_STATIC(LogTATCharacter, Log, All);

// Sets default values
ATATCharacter::ATATCharacter(const FObjectInitializer& objectInitializer)
   : Super(objectInitializer
           .SetDefaultSubobjectClass<UTATCharacterMovement>(ACharacter::CharacterMovementComponentName)
           .SetDefaultSubobjectClass<UCollisionOverlayCapsuleComponent>(ACharacter::CapsuleComponentName)
           .SetDefaultSubobjectClass<UTATCombatComponent>(AOSECharacterBase::CombatComponentName)
           .SetDefaultSubobjectClass<UTATToolSetComponent>(AOSECharacterBase::ToolSetComponentName)
           .DoNotCreateDefaultSubobject(TEXT("AbilitySystemComponent"))
           .DoNotCreateDefaultSubobject(TEXT("BaseAttributeSet"))
           .DoNotCreateDefaultSubobject(TEXT("ItemInventoryComponent")))
{
   _disguiseComponent = CreateDefaultSubobject<UTATDisguiseComponent>(TEXT("DisguiseComponent"));
   _privateSpaceCharacterComponent = CreateDefaultSubobject<UTATPrivateSpaceCharacterComponent>(TEXT("PrivateSpaceComponent"));

   _lightDetectionComponent = CreateDefaultSubobject<UTATLightDetectionCharacterComponent>(TEXT("LightDetectionComponent"));
   {
      FTATCharacterVisibilityTraceLocation eyeTraceLocation;
      eyeTraceLocation.VerticalTraceDisplacement = 0.65;
      eyeTraceLocation.HorizontalTraceDisplacement = 0.0f;
      VisibilityTraceLocations.Add(eyeTraceLocation);
   }

   {
      FTATCharacterVisibilityTraceLocation centerTraceLocation;
      centerTraceLocation.VerticalTraceDisplacement = 0.0f;
      centerTraceLocation.HorizontalTraceDisplacement = 0.0f;
      VisibilityTraceLocations.Add(centerTraceLocation);
   }

   {
      FTATCharacterVisibilityTraceLocation feetTraceLocation;
      feetTraceLocation.VerticalTraceDisplacement = -1.0f;
      feetTraceLocation.HorizontalTraceDisplacement = 0.0f;
      VisibilityTraceLocations.Add(feetTraceLocation);
   }
   _mapActorComponent = CreateDefaultSubobject<UTATMapActorComponent>(TEXT("MapActorComponent"));
   _stealthScoreComponent = CreateDefaultSubobject<UTATStealthScoreComponent>(TEXT("StealthScore"));
   _mapActorComponent->bAutoActivate = false;

   _xrayComponent = CreateDefaultSubobject<UTATXrayComponent>(TEXT("XrayComponent"));
}

void ATATCharacter::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   // TODO: move component to C++
   _interactionTargeter = FindComponentByClass<UTATInteractionTargeterComponent>(); 

   if (HasAuthority())
   {
      _tokenOwner = UUtilityAITokenOwnerGameplayTagCount::AuthorityCreate(this, DefaultAITokens, MaxAITokenDebt);
      _tokenRequester = UUtilityAITokenRequester::Create(this);
   }
}

void ATATCharacter::BeginPlay()
{
   Super::BeginPlay();

   if (ToolSetComponent)
   {
      ToolSetComponent->OnEquippedToolChanged.AddUniqueDynamic(this, &ATATCharacter::_OnEquippedToolChanged);
   }

   if (CombatComponent)
   {
      CombatComponent->OnCombatAnimationMontageEndEvent.AddUniqueDynamic(this, &ATATCharacter::_OnCombatAnimationMontageEnded);
   }
   
   UWorld* world = GetWorld();
   if (AGameStateBase* gs = GetWorld()->GetGameState())
   {
      _OnGameStateSetEvent(gs);
   }
   else
   {
      world->GameStateSetEvent.AddUObject(this, &ATATCharacter::_OnGameStateSetEvent);
   }

   if (UTATWorldMapSubsystem* worldMapSubsystem = world->GetSubsystem<UTATWorldMapSubsystem>())
   {
      worldMapSubsystem->OnMapActorRegistered.AddDynamic(this, &ATATCharacter::_OnMapActorRegistered);
   }

   if (UTATTeamsSubsystem* teamSubsystem = world->GetSubsystem<UTATTeamsSubsystem>())
   {
      teamSubsystem->RegisterCharacterToTeam(this, GetTeam());
   }

   if (UTATGameUserSettings* settings = UTATGameUserSettings::Get())
   {
      _RefreshCameraFov();
      settings->OnSettingApplied.AddUniqueDynamic(this, &ThisClass::_OnUserSettingApplied);
   }
}

void ATATCharacter::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (UTATGameUserSettings* settings = UTATGameUserSettings::Get())
   {
      settings->OnSettingApplied.RemoveAll(this);
   }
   if (UTATTeamsSubsystem* teamSubsystem = GetWorld()->GetSubsystem<UTATTeamsSubsystem>())
   {
      teamSubsystem->UnregisterCharacter(this);
   }
   Super::EndPlay(endPlayReason);
}

void ATATCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   // TODO: Audit these for ones that can be owner-only in PvP context
   DOREPLIFETIME(ATATCharacter, _cachedPlayerStateTeam);
}

void ATATCharacter::Tick(float deltaTime)
{
   Super::Tick(deltaTime);
   
   _TickIsInCombat(deltaTime);

   if (IsValid(AbilitySystemComponent))
   {
      _TickStaminaRTPC();
   }

#if ENABLE_DRAW_DEBUG
   _DebugDrawAILOSCheckLocations();
#endif // ENABLE_DRAW_DEBUG
}

void ATATCharacter::BecomeViewTarget(APlayerController* pc)
{
   Super::BecomeViewTarget(pc);

   // Reset view modifier on becoming view target to avoid discontinuities when switching back
   // (If interact camera later mirrors these modifiers, then revisit)
   const FTATFirstPersonViewModifierContext context = { this };
   for (UTATFirstPersonViewModifier* modifier : _firstPersonModifiers)
   {
      if (modifier)
      {
         modifier->Reset(context);
      }
   }
}

void ATATCharacter::PawnClientRestart()
{
   Super::PawnClientRestart();
   if (IsValid(_characterSwitch))
   {
      UAkGameplayStatics::SetSwitch(_characterSwitch, this);
   }
}

void ATATCharacter::InitializeAbilities(UOSEAbilitySystemComponent* inComponent, UAttributeBaseSet* inAttributeSet)
{
   if (bAreAbilitiesInitialized) return;

   Super::InitializeAbilities(inComponent, inAttributeSet);

   const UTATProjectSettings& settings = UTATProjectSettings::Get();

   _OnHiddenFromViewTagChanged(settings.HiddenFromViewStatusTag, inComponent->GetTagCount(settings.HiddenFromViewStatusTag));
   _OnOnlyOverlapCapsuleTagChanged(settings.OnlyOverlapCapsuleStatusTag, inComponent->GetTagCount(settings.OnlyOverlapCapsuleStatusTag));

   inComponent->RegisterGameplayTagEvent(settings.SuppressInteractionTag).AddUObject(this, &ATATCharacter::_OnSuppressInteractionTagChanged);
   inComponent->RegisterGameplayTagEvent(settings.HiddenFromViewStatusTag).AddUObject(this, &ATATCharacter::_OnHiddenFromViewTagChanged);
   inComponent->RegisterGameplayTagEvent(settings.OnlyOverlapCapsuleStatusTag).AddUObject(this, &ATATCharacter::_OnOnlyOverlapCapsuleTagChanged);
   inComponent->RegisterGameplayTagEvent(ShowOnMapTag).AddUObject(this, &ATATCharacter::_OnShowOnMapTagChanged);


   // If we entered a private space before the ASC was created, make sure we add it here
   if (HasAuthority())
   {
      _privateSpaceCharacterComponent->AuthorityUpdatePrivateSpaceTag();
   }
}

void ATATCharacter::ResetAbilities()
{
   const bool abilitiesInitialized = bAreAbilitiesInitialized;
   
   Super::ResetAbilities();

   if (abilitiesInitialized)
   {
      if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
      {
         const UTATProjectSettings& settings = UTATProjectSettings::Get();
         asc->RegisterGameplayTagEvent(settings.SuppressInteractionTag).RemoveAll(this);
         asc->RegisterGameplayTagEvent(settings.HiddenFromViewStatusTag).RemoveAll(this);
         asc->RegisterGameplayTagEvent(settings.OnlyOverlapCapsuleStatusTag).RemoveAll(this);

         if (HasAuthority())
         {
            _privateSpaceCharacterComponent->AuthorityTryForceRemovalOfPrivateSpaceEffect();
         }
         if (HasAuthority())
         {
            // Remove traversal effects and invalidate handles
            if (_sprintDurationEffectHandle.IsValid())
            {
               asc->RemoveActiveGameplayEffect(_sprintDurationEffectHandle);
               _sprintDurationEffectHandle.Invalidate();
            }
            if (_climbDurationEffectHandle.IsValid())
            {
               asc->RemoveActiveGameplayEffect(_climbDurationEffectHandle);
               _climbDurationEffectHandle.Invalidate();
            }
         }
      }
   }
}

float ATATCharacter::GetMovementMaxSpeedMultiplierToApply() const
{
   float maxSpeedMultiplier = Super::GetMovementMaxSpeedMultiplierToApply();

   // if we are holding an unstated weapon our movement speed is slowed
   if (UToolComponent* tool = _GetCurrentTool())
   {
      if (UTATItemFunctionLibrary::IsWeaponTool(tool) && !tool->IsStowed())
      {
         if (IsSprinting())
            maxSpeedMultiplier *= WeaponEquippedMaxSpeedMultiplierSprinting;
         else
            maxSpeedMultiplier *= WeaponEquippedMaxSpeedMultiplierWalking;
      }

      // ASSUMPTION: blocking only possible with a tool equipped
      if (UTATCombatFunctionLibrary::IsBlocking(this))
      {
         maxSpeedMultiplier *= BlockingMaxSpeedMultiplier;
      }
   }

   // if we are carrying a body, reduce our movement speed
   if (IsCarrying())
   {
      maxSpeedMultiplier *= CarryingMaxSpeedMultiplier;
   }

   return maxSpeedMultiplier;
}

void ATATCharacter::HandleTeamChanged()
{
   if (const IOSETeamInterface* ps = GetPlayerState<IOSETeamInterface>())
   {
      _cachedPlayerStateTeam = ps->GetTeam();

      if (_cachedPlayerStateTeam == IOSETeamInterface::kInvalidTeam)
      {
         UE_LOG(LogTATCharacter, Error,
            TEXT("ATATCharacter '%s' possessed by player state '%s' which has an invalid team set. ")
            TEXT("the team for the player state should be set before possession so it can be cached by the character"),
            *GetName(), *GetNameSafe(GetPlayerState()));
      }

      RevealOnMapIfApplicable();

      if (UTATTeamsSubsystem* teamsSubsystem = GetWorld()->GetSubsystem<UTATTeamsSubsystem>())
      {
         // Change which team we're on, which will notify the other players in the team
         teamsSubsystem->ChangeCharacterTeam(this, _cachedPlayerStateTeam);
         if (IsLocallyControlled())
         {
            // Check if we should reveal all our new teammates on the map
            TConstArrayView<TWeakObjectPtr<ATATCharacter>> allCharactersInTeam = teamsSubsystem->GetAllMembersOfTeam(_cachedPlayerStateTeam);
            for (TWeakObjectPtr<ATATCharacter> weakCharacterInTeam : allCharactersInTeam)
            {
               ATATCharacter* characterInTeam = weakCharacterInTeam.Get();
               if (characterInTeam != this && characterInTeam)
               {
                  characterInTeam->RevealOnMapIfApplicable();
               }
            }
         }
      }
   }
}

void ATATCharacter::HandleTeamMemberJoined(ATATCharacter* newTeamMember)
{
   newTeamMember->RevealOnMapIfApplicable();
}

void ATATCharacter::RevealOnMapIfApplicable() const
{
   _mapActorComponent->SetActive(_ShouldShowOnMap());
}

void ATATCharacter::ServerSetAreaInfo(const FTATAreaInfo& areaInfo)
{
   check(HasAuthority());
   _areaInfo = areaInfo;
   _BroadcastAreaInfoChanged();
}

void ATATCharacter::ServerClearAreaInfo()
{
   check(HasAuthority());
   _areaInfo = FTATAreaInfo();
   _BroadcastAreaInfoChanged();
}

void ATATCharacter::MulticastOnQuestLootChanged_Implementation(ETATInventoryUpdateEventType eventType)
{
   if(const UTATMatchSettings* TATMatchSettings = UTATMatchSettingsBase::GetTATMatchSettings<UTATMatchSettings>(GetWorld()))
   {
      // For now, we only do anything in this function if the the Match Settings are configured to reveal us when we acquire Quest Loot
      if (TATMatchSettings->DisplayAreasWithPlayersThatHaveQuestLoot)
      {
         if (const APlayerController* localController = GetWorld()->GetFirstPlayerController())
         {
            APawn* localPawn = localController->GetPawn();
            // Check if this is not our local player
            if (localPawn != this)
            {
               // If it isn't, check if they're friendly
               const EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(localPawn, this);
               if (attitude == EOSETeamAttitude::Friendly)
               {
                  // If they're friendly they are already shown on the map, so we don't need to do anything special here
                  return;
               }
               // If it isn't, they need to be revealed or hidden on the map based on whether they picked up or dropped Quest Loot
               if (eventType == ETATInventoryUpdateEventType::Add)
               {
                  _TogglePlayerHasQuestLootOnMap(true);
               }
               else if (eventType == ETATInventoryUpdateEventType::Remove)
               {
                  _TogglePlayerHasQuestLootOnMap(false);
               }
            }
         }
      }
   }
}

void ATATCharacter::_OnUserSettingApplied(FGameplayTag settingTag)
{
   if (settingTag == Tag_Settings_Video_FOV)
   {
      _RefreshCameraFov();
   }
}

void ATATCharacter::_RefreshCameraFov()
{
   float fov = 0;
   if (UTATGameUserSettings::Get()->GetSetting(Tag_Settings_Video_FOV, fov))
   {
      GetCameraComponent()->SetFieldOfView(fov);
   }
}

void ATATCharacter::_OnMapActorRegistered(const UTATMapActorComponent* mapActor)
{
   if (const APawn* registeredPawn = Cast<const APawn>(mapActor->GetRepresentedActor()))
   {
      // If the we are not the local pawn and the actual local pawn was just registered into the map...
      if (!IsLocallyControlled() && registeredPawn->IsLocallyControlled())
      {
         // Then we need to recheck if we should be visible on the map for the local player
         RevealOnMapIfApplicable();
      }
   }
}

void ATATCharacter::_OnHealthChanged(const FOnAttributeChangeData& data)
{
   // Pass through health attribute change events to the player's inventory component
   // first so it can handle things early.
   // The primary reason for this is that if we let the inventory component register
   // for health change events on its own, it gets called _after_ other handlers like
   // the toolset, which can themselves trigger inventory changes (eg. unequipping the
   // major loot tool causes the item to be dropped).
   // We need to know _why_ loot items are dropped, so let the inventory know first.
   if (HasAuthority())
   {
      if (UTATLootInventoryComponent* inventory = GetLootInventoryComponent())
      {
         inventory->AuthorityOnHealthChanged(data);
      }
   }

   Super::_OnHealthChanged(data);
}

void ATATCharacter::_OnDamageChanged(const FOnAttributeChangeData& data)
{
   // Check this before we potentially are downed by this damage
   const bool alreadyDowned = HasMatchingGameplayTag(UTATProjectSettings::Get().ConditionDownedTag);

   Super::_OnDamageChanged(data);
   // If we are unconscious but not in our final KO, then any more non-environmental damage will KO us
   if (alreadyDowned && data.NewValue > data.OldValue)
   {
      // Check if the damage source is from another character
      AActor* directInstigator = _GetInstigatorForAttributeChange(data);
      AActor* ultimateInstigator = UOSECommon::FindUltimateInstigator(directInstigator);
      AOSECharacterBase* ultimateInstigatingCharacter = UOSECommon::GetPawn<AOSECharacterBase>(ultimateInstigator);

      if (ultimateInstigatingCharacter != nullptr && ultimateInstigatingCharacter != this)
      {
         _AuthorityResetAIKnowledgeOfMyself();
         _AuthorityDropAllInventory();
         _AuthorityRemoveRelevantThiefVisionIndicatorsOnKnockout();
         
         // Check for a tool that wants to handle knockouts
         bool knockoutHandledByTool = false;
         _ForEachToolWithKnockoutHandlerInterface([&](UToolComponent* toolComponent)
         {
            if (ITATKnockoutHandlerInterface::Execute_OnCharacterKnockedOut(toolComponent, ETATKnockoutType::NonCharacterKnockout, this, nullptr))
            {
               knockoutHandledByTool = true;
            }
         });

         if (!knockoutHandledByTool)
         {
            _AuthorityPermanentKO();
         }
      }
   }
}

void ATATCharacter::_OnStealthScoreChanged()
{
}

void ATATCharacter::HandleSetPlayerState()
{
   AOSEPlayerState* previousPlayerState = OwningPlayerState;
   Super::HandleSetPlayerState();

   // NOTE: OwningPlayerState is sticky, but that probably isn't bad for this use-case
   if(previousPlayerState != OwningPlayerState)
   {
      BP_OnPlayerStateSet(previousPlayerState, OwningPlayerState);
   }
}

void ATATCharacter::OnPlayerStateChanged(APlayerState* newPlayerState, APlayerState* oldPlayerState)
{
   Super::OnPlayerStateChanged(newPlayerState, oldPlayerState);

   if (ATATPlayerState* oldTATPlayerState = Cast<ATATPlayerState>(oldPlayerState))
   {
      oldTATPlayerState->OnCharacterOutfitChanged.RemoveAll(this);
   }

   if (ATATPlayerState* newTATPlayerState = Cast<ATATPlayerState>(newPlayerState))
   {
      newTATPlayerState->OnCharacterOutfitChanged.AddDynamic(this, &ATATCharacter::_OnCurrentOutfitChanged);
   }

   if(GetNetMode() != NM_DedicatedServer)
   {
      HandleTeamChanged();
   }
}

#if WITH_EDITOR
EDataValidationResult ATATCharacter::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (IsValid(_climbDurationEffect))
   {
      if (!_climbEffectSetByCallerTag.IsValid())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] specifies _climbDurationEffect, but _climbEffectSetByCallerTag is unassigned!"), *GetName())));
         result = EDataValidationResult::Invalid;
      }
   }

   if (_characterAnimationMapping != nullptr)
   {
      int32 numErrors = _characterAnimationMapping->ValidateForCharacter(this, context);
      if (numErrors > 0)
      {
         result = EDataValidationResult::Invalid;
      }
   }

   if (HealthBlockPercentages.Num() > 0)
   {
      int32 totalHealthBlocksPercentage = 0;
      for (int32 healthBlockPercentage : HealthBlockPercentages)
      {
         totalHealthBlocksPercentage += healthBlockPercentage;
      }
      if (totalHealthBlocksPercentage != 100)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("[%s] Character Health Bar Blocks do not add up to 100 percent!"), *GetName())));
         result = EDataValidationResult::Invalid;
      }
   }

   return result;
}
#endif // WITH_EDITOR

bool ATATCharacter::CanLockPick() const
{
   return !HasMatchingGameplayTag(TAG_Status_LockpickingDisabled);
}

UTATLootInventoryComponent* ATATCharacter::GetLootInventoryComponent() const
{
   // Make sure PlayerState is replicated
   if (const ATATPlayerState* tatPs = GetPlayerState<ATATPlayerState>())
   {
      return tatPs->GetLootInventoryComponent();
   }

   return nullptr;
}

bool ATATCharacter::CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor, const bool* wasVisible, int32* userData) const
{
   FHitResult hitResult;
   numberOfLoSChecksPerformed = 0;
   
   float totalImportance = 0.0f;
   float importanceOfSuccessfulHits = 0.0f;

   FVisibilityTraceLocationArray locationsToCheck;
   _GetAILOSCheckLocations(locationsToCheck);

   for (const FVisibilityTraceLocationAndImportance& locationAndImportance : locationsToCheck)
   {
      const bool canBeSeen = UOSEAIFunctionLibrary::SightSenseLineTrace(hitResult, observerLocation, this, ignoreActor, &locationAndImportance.Location);

#if ENABLE_DRAW_DEBUG
      // Draw a line showing the LoS trace: red means blocked, white means visible
      if (CharacterCVars::DebugDrawAILineOfSightChecks)
      {
         DrawDebugLine(GetWorld(), observerLocation, locationAndImportance.Location, canBeSeen ? FColor::White : FColor::Red, false, 0.0f);
      }
#endif

      if (canBeSeen)
      {
         importanceOfSuccessfulHits += locationAndImportance.RelativeImportance;
      }
      totalImportance += locationAndImportance.RelativeImportance;
      numberOfLoSChecksPerformed += 1;
   }

   // If any succeed, then we have been seen!
   if (importanceOfSuccessfulHits > 0.0f)
   {
      outSeenLocation = GetActorLocation();
      // TODO: Consider reporting _which_ traces succeeded in the strength somehow...
      outSightStrength = importanceOfSuccessfulHits / totalImportance;
      return true;
   }

   // Not enough traces succeeded, so we cannot be seen.
   outSightStrength = 0.0f;
   return false; 
}

bool ATATCharacter::CanLightRayHitActor(
   const FVector& fromLocation,
   const AActor* actorToIgnore, 
   int& outNumberOfLoSChecksPerformed,
   float& outMinHitDistance) const
{
   outMinHitDistance = FLT_MAX;
   
   FVisibilityTraceLocationArray locationsToCheck;
   _GetAILOSCheckLocations(locationsToCheck);

   for (const FVisibilityTraceLocationAndImportance& locationAndImportance : locationsToCheck)
   {
      FHitResult hitResult;
      const bool canBeSeen = UOSEAIFunctionLibrary::SightSenseLineTrace(hitResult, fromLocation, this, actorToIgnore, &locationAndImportance.Location);

#if ENABLE_DRAW_DEBUG
      // Draw a line showing the LoS trace: red means blocked, white means visible
      if (CharacterCVars::DebugDrawAILightDetectionChecks)
      {
         DrawDebugLine(GetWorld(), fromLocation, locationAndImportance.Location, canBeSeen ? FColor::White : FColor::Red, false, 0.0f);
      }
#endif

      if (canBeSeen)
      {
         const float distance = FVector::Distance(hitResult.TraceStart, hitResult.TraceEnd);
         outMinHitDistance = FMath::Min(outMinHitDistance, distance);
      }
      outNumberOfLoSChecksPerformed += 1;
   }
   if(outMinHitDistance < FLT_MAX)
   {
      return true;
   }
   return false;
}

float ATATCharacter::GetCurrentLightIntensityPlusMinimumValue() const
{
   if(_lightDetectionComponent)
   {
      return _lightDetectionComponent->GetCurrentLightIntensityPlusMinimumValue();
   }
   return 1.f;
}

float ATATCharacter::GetActualLightIntensityFromLightSources() const
{
   if(_lightDetectionComponent)
   {
      return _lightDetectionComponent->GetActualLightIntensityFromLightSources();
   }
   return 1.f;
}

void ATATCharacter::AuthorityOnEnterVisibleByActor(AActor* viewingActor)
{
   check(!_authorityViewingActors.Contains(viewingActor));
   _authorityViewingActors.AddUnique(viewingActor);
}

void ATATCharacter::AuthorityOnExitVisibleByActor(AActor* viewingActor)
{
   _authorityViewingActors.Remove(viewingActor);

   // Remove any null entries in _viewingActors.
   _authorityViewingActors.RemoveAll([&](AActor* maybeNullActor)
   {
      return !IsValid(maybeNullActor);
   });
}

bool ATATCharacter::CanBeSneakAttackedByActor(AActor* actor) const
{
   return BP_CanBeSneakAttackedByActor(actor);
}

void ATATCharacter::OnSneakAttackedByActor(AActor* actor)
{
   UE_LOG(LogTATCharacter, Verbose, TEXT("Player '%s' was sneak attacked by '%s'"), *GetName(), *GetNameSafe(actor));
}

bool ATATCharacter::CanBeCounterAttackedByActor(AActor* actor) const
{
   return UTATCombatFunctionLibrary::IsDefenderVulnerableToCounterByAttacker(actor, this);
}

void ATATCharacter::OnCounterAttackedByActor(AActor* actor)
{
   UE_LOG(LogTATCharacter, Verbose, TEXT("Player '%s' was counter attacked by '%s'"), *GetName(), *GetNameSafe(actor));

   const UTATCombatSettings& combatSettings = UTATCombatSettings::Get();
   AbilitySystemComponent->RemoveActiveEffectsWithAppliedTags(combatSettings.IsVulnerableToCounterTag.GetSingleTagContainer());
}

bool ATATCharacter::BP_CanBeSneakAttackedByActor_Implementation(AActor* actor) const
{
   if (actor)
   {
      return UTATCombatFunctionLibrary::CanBeSneakAttackedFromPosition(this, actor->GetActorLocation());
   }
   else
   {
      return false;
   }
}

UAnimMontage* ATATCharacter::GetCharacterMontage(const FGameplayTag& animationTag) const
{
   // Allow the disguise component to override what montage we play
   if (_disguiseComponent != nullptr)
   {
      if (UAnimMontage* montage = _disguiseComponent->GetDisguisedCharacterMontage(animationTag))
      {
         return montage;
      }
   }

   if (_characterAnimationMapping)
   {
      return _characterAnimationMapping->LookupMontageByTag(animationTag);
   }
   else
   {
      UE_LOG(LogTATCharacter, Warning, TEXT("TATCharacter '%s' is missing _characterAnimationMapping, despite GetCharacterMontage called on it"), *GetName());
      return nullptr;
   }
}

USkeletalMesh* ATATCharacter::GetDisguiseTargetMesh(ETATDisguiseMeshType meshType, TArray<UMaterialInterface*>& outOverrideMaterials) const
{
   switch (meshType)
   {
   case ETATDisguiseMeshType::Skeleton:
      return ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(GetMesh(), outOverrideMaterials);
   case ETATDisguiseMeshType::ThirdPersonBody:
      return ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(GetMesh3P_Body(), outOverrideMaterials);
   case ETATDisguiseMeshType::ThirdPersonHead:
      return ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(GetMesh3P_Head(), outOverrideMaterials);
   case ETATDisguiseMeshType::FirstPersonLowerBody:
      return ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(GetMesh1P_LowerBody(), outOverrideMaterials);
   case ETATDisguiseMeshType::FirstPersonUpperBody:
      return ITATDisguiseTargetInterface::GetSkeletalMeshAndMaterials(GetMesh1P_UpperBody(), outOverrideMaterials);
   default:
      checkNoEntry();
      break;
   }
   return nullptr;
}

TSubclassOf<UAnimInstance> ATATCharacter::GetDisguiseTargetThirdPersonAnimClass() const
{
   if (USkeletalMeshComponent* bodyMeshComp = GetMesh())
   {
      return bodyMeshComp->GetAnimClass();
   }
   return nullptr;
}

TSubclassOf<UAnimInstance> ATATCharacter::GetDisguiseTargetAnimSetLayer() const
{
   return _GetAnimSetForTag(TAG_AnimSet_Disguise);
}

FTATDisguiseMovementParams ATATCharacter::GetDisguiseTargetMovementParams() const
{
   FTATDisguiseMovementParams result{};
   if (UTATCharacterMovement* moveComp = GetCharacterMovement<UTATCharacterMovement>())
   {
      result.MaxWalkSpeed = moveComp->MaxWalkSpeed;
      result.MaxAcceleration = moveComp->MaxAcceleration;
   }
   return result;
}

ETATDisguiseTargetType ATATCharacter::GetDisguiseTargetType() const
{
   return ETATDisguiseTargetType::None;
}

bool ATATCharacter::AuthorityIsBeingViewed() const
{
   ensure(HasAuthority());

   return _authorityViewingActors.ContainsByPredicate([](const AActor* viewingActor)
   {
      return IsValid(viewingActor);
   });
}

bool ATATCharacter::CanWallClimb() const
{
   if (!Super::CanWallClimb())
   {
      return false;
   }

   return !HasAnyMatchingGameplayTags(WallClimbSuppressionTags);
}

bool ATATCharacter::CanSprint() const
{
   if (!Super::CanSprint())
   {
      return false;
   }
   
   // Skip checks until the ASC replicates
   // TODO: when we actually decide to do client-fudging work to prevent correction, this should live in the ASC
   if (AbilitySystemComponent)
   {
      const float currentStamina = _GetStaminaAttributeValue();

      // Don't let the player start sprinting until stamina regenerates above a minimum threshold
      // But if they're already sprinting, let them deplete stamina to 0
      const float staminaThreshold = IsSprinting() ? 0.f : _minimumStaminaToBeginSprinting;

      if (currentStamina <= staminaThreshold)
      {
         return false;
      }
   }

   return !HasAnyMatchingGameplayTags(SprintSuppressionTags);
}

void ATATCharacter::SprintRequest()
{
   Super::SprintRequest();

   const float staminaValue = _GetStaminaAttributeValue();
   if (staminaValue <= _minimumStaminaToBeginSprinting)
   {
      FGameplayCueParameters params;
      UOSEAbilityFunctionLibrary::ExecuteNonReplicatedGameplayCueOnActorWithParams(this, _sprintRequestWithoutStaminaGameplayCue, params);
   }
}

void ATATCharacter::OnStartSprinting_Implementation()
{
   Super::OnStartSprinting_Implementation();

   if (HasAuthority())
   {
      if (_sprintDurationEffect)
      {
         check(AbilitySystemComponent);
         FGameplayEffectContextHandle effectContext = AbilitySystemComponent->MakeEffectContext();
         FGameplayEffectSpec spec(_sprintDurationEffect->GetDefaultObject<UGameplayEffect>(), effectContext, UGameplayEffect::INVALID_LEVEL);

         // Apply effect and cache handle for removal when player stops sprinting
         _sprintDurationEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(spec);
      }
   }
}

void ATATCharacter::OnStopSprinting_Implementation()
{
   Super::OnStopSprinting_Implementation();

   if (HasAuthority())
   {
      if (_sprintDurationEffectHandle.IsValid())
      {
         check(AbilitySystemComponent);

         // Remove effect and invalidate handle
         AbilitySystemComponent->RemoveActiveGameplayEffect(_sprintDurationEffectHandle);
         _sprintDurationEffectHandle.Invalidate();
      }
   }
}

bool ATATCharacter::CanScramble() const
{
   if (!Super::CanScramble())
   {
      return false;
   }

   // Skip checks until the ASC replicates
   // TODO: when we actually decide to do client-fudging work to prevent correction, this should live in the ASC
   if (AbilitySystemComponent && _GetStaminaAttributeValue() <= 0.f)
   {
      return false;
   }
   return true;
}

void ATATCharacter::OnStartScrambling_Implementation(const FHitResult& initialClimbImpact)
{
   Super::OnStartScrambling_Implementation(initialClimbImpact);

   if (HasAuthority())
   {
      if (_climbDurationEffect)
      {
         check(AbilitySystemComponent);
         FGameplayEffectContextHandle effectContext = AbilitySystemComponent->MakeEffectContext();
         FGameplayEffectSpec spec(_climbDurationEffect->GetDefaultObject<UGameplayEffect>(), effectContext, UGameplayEffect::INVALID_LEVEL);

         // Must have attribute set-by-caller tag assigned
         if (!_climbEffectSetByCallerTag.IsValid())
         {
            UE_LOG(LogTATCharacter, Error, TEXT("Could not apply _climbDurationEffect due to to unassigned _climbEffectSetByCallerTag!"));
            return;
         }

         if (const UPhysicalMaterial* physMat = initialClimbImpact.PhysMaterial.Get())
         {
            // Set climb stamina burn for effect, using surface override if specified (or default value otherwise)
            float surfaceClimbStaminaBurn = _GetClimbStaminaBurnForSurface(physMat->SurfaceType, spec.Period);
            spec.SetSetByCallerMagnitude(_climbEffectSetByCallerTag, surfaceClimbStaminaBurn);

            // Apply effect and cache handle for removal when player stops sprinting
            _climbDurationEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(spec);

            // Bind to physmat change in CMC 
            UOSECharacterMovement* oseMovement = CastChecked<UOSECharacterMovement>(GetCharacterMovement());
            oseMovement->OnPhysicalMaterialChanged.AddUniqueDynamic(this, &ATATCharacter::_AuthorityOnPhysicalMaterialChangedWhileClimbing);
         }
      }
   }
}

void ATATCharacter::OnStopScrambling_Implementation(const FHitResult& initialClimbImpact)
{
   Super::OnStopScrambling_Implementation(initialClimbImpact);

   if (HasAuthority())
   {
      // Unbind from physmat change in CMC
      UOSECharacterMovement* oseMovement = CastChecked<UOSECharacterMovement>(GetCharacterMovement());
      oseMovement->OnPhysicalMaterialChanged.RemoveDynamic(this, &ATATCharacter::_AuthorityOnPhysicalMaterialChangedWhileClimbing);

      if (_climbDurationEffectHandle.IsValid())
      {
         check(AbilitySystemComponent);

         // Remove effect and invalidate handle
         AbilitySystemComponent->RemoveActiveGameplayEffect(_climbDurationEffectHandle);
         _climbDurationEffectHandle.Invalidate();
      }
   }
}

void ATATCharacter::Crouch(bool bClientSimulation)
{
   _wantsToCrouch = true;
   Super::Crouch(bClientSimulation);
}

void ATATCharacter::UnCrouch(bool bClientSimulation)
{
   _wantsToCrouch = false;
   _UpdateUnCrouch(bClientSimulation);
}

namespace CharacterHelpers
{
   /// Generic helper function for playing a montage on a skeletal mesh component.
   /// This is pretty much just copy-pasted from ACharacter::PlayAnimMontage
   float PlayMontage(USkeletalMeshComponent* skeletalMeshComp, UAnimMontage* montage, float playRate, FName startSection)
   {
      UAnimInstance* animInstance = (skeletalMeshComp) ? skeletalMeshComp->GetAnimInstance() : nullptr;
      if (montage && animInstance)
      {
         float const duration = animInstance->Montage_Play(montage, playRate);

         if (duration > 0.f)
         {
            // Start at a given Section.
            if (startSection != NAME_None)
            {
               animInstance->Montage_JumpToSection(startSection, montage);
            }

            return duration;
         }
      }

      return 0.f;
   }
}

float ATATCharacter::PlayAnimMontage(UAnimMontage* animMontage, float inPlayRate, FName startSectionName)
{
   // When disguised, if our first-person upper body mesh does not use a leader pose component, play the montage on it directly.
   if (_disguiseComponent && _disguiseComponent->IsDisguiseActive() && Mesh1P_UpperBody && Mesh1P_UpperBody->LeaderPoseComponent == nullptr)
   {
      CharacterHelpers::PlayMontage(Mesh1P_UpperBody, animMontage, inPlayRate, startSectionName);
   }

   return Super::PlayAnimMontage(animMontage, inPlayRate, startSectionName);
}

uint8 ATATCharacter::GetTeam() const
{
   if (_disguiseComponent->IsDisguiseActive())
   {
      return _disguiseComponent->GetTeamForDisguise();
   }

   // TODO(Post-PAM): Probably remove, as this would likely interfere with coop if actually used
   if(HasMatchingGameplayTag(TAG_STATUS_PRIVATESPACE_HOSTILEINTRUDER))
   {
      // This is only used for off-limit zones now - we may want to remove this in the future however this is the
      // cleanest way to make you hostile to _all_ other teams.
      return UTATProjectSettings::GetTeamAssignmentForCharacterType(ETATTeamCharacterType::Intruder);
   }
   // If we've cached the team from our most-recently-controlling player state
   // use that here. This will allow us to continue to have the correct team
   // even if we are no longer being controlled by that same controller
   if (_cachedPlayerStateTeam != IOSETeamInterface::kInvalidTeam)
   {
      return _cachedPlayerStateTeam;
   }
   return Super::GetTeam();
}

void ATATCharacter::ForceCrouch(bool bShouldCrouch)
{
   _forceCrouch = bShouldCrouch;
   if (_forceCrouch)
   {
      Super::Crouch(false);
   }
   else
   {
      _UpdateUnCrouch(false);
   }
}

void ATATCharacter::_UpdateUnCrouch(bool bClientSimulation)
{
   if (!_forceCrouch && !_wantsToCrouch)
   {
      Super::UnCrouch(bClientSimulation);
   }
}

void ATATCharacter::_AuthoritySpawnKOGlyph()
{
   check(HasAuthority());

   UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>();
   check(IsValid(thiefVisionSubsystem));

   FTransform spawnTransform(GetActorLocation());
   thiefVisionSubsystem->AuthoritySpawnThiefVisionIndicator(KnockedOutGlyphIndicatorType, spawnTransform);
}

void ATATCharacter::_AuthorityOnPhysicalMaterialChangedWhileClimbing(const FOSEMovementMaterialContext& currentContext, const FOSEMovementMaterialContext& previousContext)
{
   check(HasAuthority());
   check(AbilitySystemComponent);

   // Annoyingly, despite being named OnPhysicalMaterialChanged, the delegate can be called when other members besides PhysicalMaterial change in FOSEMovementMaterialContext
   if (previousContext.PhysicalMaterial == currentContext.PhysicalMaterial)
   {
      return;
   }

   if (const UPhysicalMaterial* physMat = currentContext.PhysicalMaterial.Get())
   {
      if (!_climbEffectSetByCallerTag.IsValid())
      {
         UE_LOG(LogTATCharacter, Error, TEXT("_AuthorityOnPhysicalMaterialChangedWhileClimbing() | could not update climb stamina effect magnitude due to unspecified _climbEffectSetByCallerTag!"));
         return;
      }

      // Update set-by-caller magnitude for new surface type, if effect present (could have been suppressed by Status.StaminaConsumptionDisabled tag presence)
      if (_climbDurationEffectHandle.IsValid())
      {
         const FActiveGameplayEffect* activeClimbEffect = AbilitySystemComponent->GetActiveGameplayEffect(_climbDurationEffectHandle);
         check(activeClimbEffect);

         const float surfaceClimbStaminaBurn = _GetClimbStaminaBurnForSurface(physMat->SurfaceType, activeClimbEffect->GetPeriod());
         AbilitySystemComponent->UpdateActiveGameplayEffectSetByCallerMagnitude(_climbDurationEffectHandle, _climbEffectSetByCallerTag, surfaceClimbStaminaBurn);
      }
   }
}

float ATATCharacter::_GetClimbStaminaBurnForSurface(EPhysicalSurface physicalSurface, float effectPeriod) const
{
   // Return surface-specific stamina burn override if specified
   if (const FScalableFloat* staminaBurnSurfaceOverride = _climbStaminaBurnSurfaceOverrides.Find(physicalSurface))
   {
      return staminaBurnSurfaceOverride->GetValue() * effectPeriod;
   }

   // Otherwise fall back to default
   return _defaultClimbStaminaBurnPerSecond.GetValue() * effectPeriod;
}

void ATATCharacter::OnStartMantling_Implementation()
{
   Super::OnStartMantling_Implementation();

   // Fine to also do this on non-authority, if the abilities allow it
   if (UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      if (_cancelOnMantleAbilityTag.IsValid())
      {
         FGameplayTagContainer tagsToCancel = _cancelOnMantleAbilityTag.GetSingleTagContainer();
         asc->CancelAbilities(&tagsToCancel);
      }
   }
}


void ATATCharacter::_GetAILOSCheckLocations(FVisibilityTraceLocationArray& locationsToCheck) const
{
   UCapsuleComponent* capsule = GetCapsuleComponent();
   FVector capsuleLocation = capsule->GetComponentLocation();
   float capsuleHalfHeight = capsule->GetScaledCapsuleHalfHeight();
   float capsuleRadius = capsule->GetScaledCapsuleRadius();

   const FVector actorUp = GetActorUpVector();
   const FVector actorRight = GetActorRightVector();

   for (const FTATCharacterVisibilityTraceLocation& visibilityTraceLocation : VisibilityTraceLocations)
   {
      FVector traceLocation = capsuleLocation
         + (actorUp * capsuleHalfHeight * visibilityTraceLocation.VerticalTraceDisplacement)
         + (actorRight * capsuleRadius * visibilityTraceLocation.HorizontalTraceDisplacement);

      FVisibilityTraceLocationAndImportance locationAndImportance;
      locationAndImportance.Location = traceLocation;
      locationAndImportance.RelativeImportance = visibilityTraceLocation.RelativeImportance;
      locationsToCheck.Add(locationAndImportance);
   }
}

void ATATCharacter::_DebugDrawAILOSCheckLocations() const
{
#if ENABLE_DRAW_DEBUG
   if (CharacterCVars::DebugDrawAILineOfSightLocations)
   {
      FVisibilityTraceLocationArray locationsAndImportances;
      _GetAILOSCheckLocations(locationsAndImportances);

      for (const FVisibilityTraceLocationAndImportance& locationAndImportance : locationsAndImportances)
      {
         DrawDebugSphere(GetWorld(), locationAndImportance.Location, 15, 16, FColor::Orange, false);
      }
   }
#endif // ENABLE_DRAW_DEBUG
}

void ATATCharacter::_AuthorityDropAllInventory()
{
   check(HasAuthority());

   // We will also drop it on health changed, but make sure we do this first to avoid races with what comes later
   if (UTATLootInventoryComponent* lootInventory = GetLootInventoryComponent())
   {
      lootInventory->AuthorityDropLootOnKO();
   }

   // Drop all keys
   if (UTATItemInventoryComponent* itemInventory = GetTATItemInventory())
   {
      UTATItemFunctionLibrary::DropItemsForKO(itemInventory);
   }
}

void ATATCharacter::_AuthorityRemoveRelevantThiefVisionIndicatorsOnKnockout()
{
   check(HasAuthority());

   UTATThiefVisionSubsystem* thiefVisionSubsystem = GetWorld()->GetSubsystem<UTATThiefVisionSubsystem>();
   check(IsValid(thiefVisionSubsystem));

   FTATThiefVisionIndicatorQuery query{};
   query.FilterByInstigator = true;
   query.Instigator = this;

   query.FilterByIndicatorType = true;
   thiefVisionSubsystem->FindThiefVisionIndicatorTypes(query.IndicatorTypes, [](FGameplayTag ty, const FTATThiefVisionIndicatorPrecomputedConfig& cfg)
   {
      return cfg.AutoRemoveOnInstigatorKnockout;
   });

   if (query.IndicatorTypes.Num() == 0)
   {
      return;
   }

   const int32 numRemovedIndicators = thiefVisionSubsystem->AuthorityRemoveThiefVisionIndicatorsMatchingQuery(query);

   UE_LOG(LogTATCharacter, Verbose, TEXT("Removed %i thief vision indicators owned by %s due to knockout event"), numRemovedIndicators, *GetNameSafe(this))
}

void ATATCharacter::_AuthorityPermanentKO()
{
   check(HasAuthority());

   if (FinalKOAbility != nullptr)
   {
      UAbilitySystemComponent* asc = GetAbilitySystemComponent();
      check(asc);
      asc->TryActivateAbilityByClass(FinalKOAbility);
   }
   else
   {
      UE_LOG(LogTATCharacter, Error, TEXT("'%s': expected to have valid FinalKOAbility"), *GetName());
   }
}

void ATATCharacter::_ForEachToolWithKnockoutHandlerInterface(TFunctionRef<void(UToolComponent*)> callback) const
{
   TScriptInterface<IToolSetInterface> toolSet = GetToolSetInterface();
   if (!toolSet)
   {
      return;
   }

   for (int32 i = 0; i < toolSet->GetNumTools(); i++)
   {
      UToolComponent* toolComponent = toolSet->GetToolAtIndex(i);
      if (toolComponent != nullptr && toolComponent->Implements<UTATKnockoutHandlerInterface>())
      {
         callback(toolComponent);
      }
   }
}

FVector ATATCharacter::GetMoveGoalOffset(const AActor* movingActor) const
{
   if(const ATATAIController* controller = Cast<ATATAIController>(movingActor))
   {
      if(const AOSECharacterBase* characterBase = Cast<AOSECharacterBase>(controller->GetPawn()))
      {
         const ECombatPosition combatPosition = controller->GetRequestedCombatPosition();
         return _combatPositioningComponent->GetOffsetForCharacter(characterBase, combatPosition);
      }
   }
   return Super::GetMoveGoalOffset(movingActor);
}

void ATATCharacter::GetMoveGoalReachTest(const AActor* movingActor,
   const FVector& moveOffset,
   FVector& goalOffset,
   float& goalRadius,
   float& goalHalfHeight) const
{
   Super::GetMoveGoalReachTest(movingActor, moveOffset, goalOffset, goalRadius, goalHalfHeight);
   goalOffset = moveOffset;
}

void ATATCharacter::OnHealthChanged_Implementation(float newValue, float oldValue)
{
   Super::OnHealthChanged_Implementation(newValue, oldValue);
}

void ATATCharacter::_OnIsCharacterReadyChanged(bool isReady)
{
   if (isReady)
   {
      // we have a tool set if we're ready
      check(ToolSetComponent);

      _recentlyEquippedToolClass = nullptr;
      _recentlyEquippedWeaponClass = nullptr;

      const UTATProjectSettings& settings = *GetDefault<UTATProjectSettings>();
      for(int toolIdx = 0; toolIdx < ToolSetComponent->GetNumTools(); ++toolIdx)
      {
         if (UToolComponent* tool = ToolSetComponent->GetToolAtIndex(toolIdx))
         {
            // ASSUMPTION: the first one in the list should be our "recently equipped" one to
            // start with (it's the first one assigned in the tools list on the character)
            if (UTATItemFunctionLibrary::IsWeaponTool(tool))
            {
               if (!_recentlyEquippedWeaponClass)
               {
                  _recentlyEquippedWeaponClass = tool->GetClass();
               }
            }
            else if (!_recentlyEquippedToolClass)
            {
               _recentlyEquippedToolClass = tool->GetClass();
            }
         }
         if (_recentlyEquippedToolClass && _recentlyEquippedWeaponClass)
            break;
      }
   }
}

float ATATCharacter::_GetStaminaAttributeValue() const
{
   check(AbilitySystemComponent);
   
   // Reject sprinting if stamina reaches 0
   bool attributeFound = false;
   const float staminaValue = AbilitySystemComponent->GetGameplayAttributeValue(UTATStaminaAttributeSet::GetStaminaAttribute(), attributeFound);
   if (attributeFound)
   {
      return staminaValue;
   }
   else
   {
      return -1.f;
   }
}

void ATATCharacter::OnRep_PlayerState()
{
   Super::OnRep_PlayerState();

   if (const ATATPlayerState* tatPlayerState = GetPlayerState<ATATPlayerState>())
   {
      // NOTE: Doing this here in both PossessedBy and OnRep_PlayerState
      //       Putting it in HandleSetPlayerState runs into ordering issues
      //       But this code will be removed anyways
      if (!bCreatedStaticItemInventoryComponent && tatPlayerState->GetTATItemInventory())
      {
         InitializeItemInventory(tatPlayerState->GetTATItemInventory());
      }

      _LoadAndApplyCharacterOutfit(tatPlayerState->GetCurrentOutfitLoadout());
   }
}

void ATATCharacter::_TickIsInCombat(float deltaTime)
{
   // "in combat" should be able to be inferred locally, on the server, and for simulated proxies via externally replicated state
   if(CombatComponent)
   {
      const TArray<FCombatActorInfo>& aggressiveCharacters = CombatComponent->GetAggressiveCharacterList();
      const bool isInCombat = aggressiveCharacters.Num() > 0;
      _SetIsInCombat(isInCombat);

      // TODO: A falloff timer after we're no longer being pursued to drop combat?
   }
}

void ATATCharacter::_TickStaminaRTPC()
{
   if (!IsValid(_staminaRtpc))
   {
      UE_LOG(LogTATCharacter, Verbose, TEXT("[%s] _TickStaminaRTPC() could not update stamina RTPC due to unassigned _staminaRtpc!"), *GetName());
      return;
   }

   const float staminaValue = _GetStaminaAttributeValue();
   const int interpolationTimeMs = 0;
   UAkGameplayStatics::SetRTPCValue(_staminaRtpc, staminaValue, interpolationTimeMs, this);
}

//---------------------------------------------------------------------------------------
// IUtilityAIBehaviorTargetInterface
//---------------------------------------------------------------------------------------

void ATATCharacter::AuthorityOnEnterTargetedByBehavior_Implementation(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior)
{
   if (UOSEAIFunctionLibrary::IsAggressiveState(behavior) == false)
   {
      return;
   }
   // base class passes along to the combat component
   if (CombatComponent)
   {
      CombatComponent->AddTargetingCharacter(aiCharacter);
   }
}

void ATATCharacter::AuthorityOnExitTargetedByBehavior_Implementation(AOSECharacterBase* aiCharacter, UUtilityAIStateBase* behavior)
{
   if (UOSEAIFunctionLibrary::IsAggressiveState(behavior) == false)
   {
      return;
   }
   // base class passes along to the combat component
   if (CombatComponent)
   {
      CombatComponent->RemoveTargetingCharacter(aiCharacter);
   }
}

void ATATCharacter::AuthorityOnEnterTargetedByGoal_Implementation(AOSECharacterBase* aiCharacter,
   UUtilityAIStateBase* behavior)
{
   if (UOSEAIFunctionLibrary::IsAggressiveState(behavior) == false)
   {
      return;
   }
   _combatPositioningComponent->AddTargetingCharacter(aiCharacter);
}

void ATATCharacter::AuthorityOnExitTargetedByGoal_Implementation(AOSECharacterBase* aiCharacter,
   UUtilityAIStateBase* behavior)
{
   if (UOSEAIFunctionLibrary::IsAggressiveState(behavior) == false)
   {
      return;
   }
   _combatPositioningComponent->RemoveTargetingCharacter(aiCharacter);
}


UToolComponent* ATATCharacter::_GetCurrentTool() const
{
   if (ToolSetComponent)
      return ToolSetComponent->GetCurrentTool();
   return nullptr;
}

void ATATCharacter::_SetIsInCombat(bool newIsInCombat)
{
   if (newIsInCombat != _isInCombat)
   {
      _isInCombat = newIsInCombat;
      OnIsInCombatChanged.Broadcast(_isInCombat);
   }
}

void ATATCharacter::_RestartRecentlyUsedWeaponTimer()
{
   if (IsLocallyControlled())
   {
      _recentlyUsedWeaponTimeRemaining = RecentlyUsedWeaponTimeSeconds;
   }
}

void ATATCharacter::_OnSuppressInteractionTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   if (_interactionTargeter)
   {
      _interactionTargeter->SetTargetingSuppressed(newTagCount > 0);
   }
}

void ATATCharacter::_OnHiddenFromViewTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   bool newIssHiddenFromView = (newTagCount > 0);
   if (_isHiddenFromView != newIssHiddenFromView)
   {
      if (newIssHiddenFromView)
      {
         SetActorTickEnabled(false);
         SetActorHiddenInGame(true);
      }
      else
      {
         SetActorTickEnabled(true);
         SetActorHiddenInGame(false);
      }

      _isHiddenFromView = newIssHiddenFromView;
   }
}

void ATATCharacter::_OnOnlyOverlapCapsuleTagChanged(const FGameplayTag tag, int32 newTagCount)
{
   bool newOnlyOverlap = (newTagCount > 0);
   if (_isOnlyOverlapCapsule != newOnlyOverlap)
   {
      auto overlayCapsule = CastChecked<ICollisionOverlayInterface>(GetCapsuleComponent());
      check(overlayCapsule);
      if (newOnlyOverlap)
      {
         FCollisionResponseContainer overlay(ECR_Overlap);
         overlay.SetResponse(COLLISION_INTERACT, ECR_Ignore); // explicitly ignore interact collision, since interact traces include overlaps
         overlayCapsule->AddCollisionOverlay(tag, overlay);
      }
      else
      {
         overlayCapsule->RemoveCollisionOverlayByKey(tag);
      }

      // If this is true, only the capsule is allowed to have collision. Assume for now (later validate?) that only the 3p mesh
      // has collision enabled in addition to the capsule. If any other code wants to write to this value, add an intermediate
      // counter, or a similar collision overlay (although that limits subclasses).
      GetMesh()->SetCollisionEnabled(newOnlyOverlap ? ECollisionEnabled::NoCollision : ECollisionEnabled::QueryOnly);

      // Movement must be disabled, or it will just fall through everything
      GetCharacterMovement()->SetMovementMode(newOnlyOverlap ? MOVE_None : MOVE_Falling);

      _isOnlyOverlapCapsule = newOnlyOverlap;
   }
}

void ATATCharacter::_OnGameStateSetEvent(AGameStateBase* gameState)
{
   if (ATATGameState* tatsGS = Cast<ATATGameState>(gameState))
   {
      tatsGS->OnGameFrozenChanged.AddUniqueDynamic(this, &ATATCharacter::_OnIsGameFrozenChanged);
      _OnIsGameFrozenChanged(tatsGS->IsGameFrozen());
   }
}

void ATATCharacter::_OnRep_ViewingActors()
{
   _BroadcastViewingActorsChanged();
}

void ATATCharacter::_BroadcastViewingActorsChanged()
{
   UE_LOG(LogTATCharacter, Verbose, TEXT("%s has %d actors viewing them"), *GetName(), _authorityViewingActors.Num());
   OnViewingActorsChanged.Broadcast(_authorityViewingActors);
}

void ATATCharacter::_OnCombatAnimationMontageEnded()
{
   _RestartRecentlyUsedWeaponTimer();
}

void ATATCharacter::PlayDip(float dipSpeed, float dipStrength)
{
   if (CharacterCVars::ReducedCameraBounce)
   {
      return;
   }
   for(UTATFirstPersonViewModifier* modifier : _firstPersonModifiers)
   {
      if(auto* dipModifier = Cast<UTATFirstPersonViewModifier_Dip>(modifier))
      {
         dipModifier->PlayDip(dipSpeed, dipStrength);
      }
   }
}

void ATATCharacter::_ModifyCameraTransform(float deltaTime, FTransform& transform)
{
   FTransform accumulator;
   FTATFirstPersonViewModifierContext context { this };
   context.AllowBouncing = !CharacterCVars::ReducedCameraBounce;
   for(UTATFirstPersonViewModifier* modifier : _firstPersonModifiers)
   {
      if(modifier)
      {
         modifier->ModifyCamera(context, deltaTime, accumulator);
      }
   }

   transform = accumulator * transform;
}

void ATATCharacter::_ModifyFirstPersonMeshViewTransform(float deltaTime, FTransform& transform)
{
   FTransform accumulator;
   FTATFirstPersonViewModifierContext context { this };
   context.AllowBouncing = !CharacterCVars::ReducedCameraBounce;
   for(UTATFirstPersonViewModifier* modifier : _firstPersonModifiers)
   {
      if(modifier)
      {
         modifier->ModifyFirstPersonMeshView(context, deltaTime, accumulator);
      }
   }

   transform = accumulator * transform;
}

void ATATCharacter::_TogglePlayerHasQuestLootOnMap(bool hasQuestLoot) const
{
   FTATMapRepresentationData mapData = _mapActorComponent->GetMapRepresentationData();
   if (mapData.ShowGeneralAreaOnQuestLootObtained)
   {
      _mapActorComponent->SetGeneralAreaVisible(hasQuestLoot);
      _mapActorComponent->SetActive(hasQuestLoot);
   }
}

void ATATCharacter::_BroadcastAreaInfoChanged()
{
   UE_LOG(LogTATCharacter, Verbose, TEXT("Area info changed: %s (%s)"), *_areaInfo.Name.ToString(), *_areaInfo.Tag.ToString());
   OnAreaInfoChanged.Broadcast(_areaInfo);
}

void ATATCharacter::InitializeItemInventory(UItemInventoryComponent* itemInventoryComponent)
{
   Super::InitializeItemInventory(itemInventoryComponent);

   _tatItemInventory = Cast<UTATItemInventoryComponent>(ItemInventoryComponent);
   if (_tatItemInventory && HasAuthority())
   {
      check(bAreAbilitiesInitialized);
      _tatItemInventory->AuthoritySetToolset(GetToolSetInterface());
   }
}

void ATATCharacter::_OnCurrentOutfitChanged(ATATPlayerState* ps, const TArray<FTATCharacterLoadoutEntry>& outfitLoadout)
{
   _LoadAndApplyCharacterOutfit(outfitLoadout);
}

void ATATCharacter::_LoadAndApplyCharacterOutfit(const TArray<FTATCharacterLoadoutEntry>& outfitLoadout)
{
   TSoftObjectPtr<UDataTable> outfitMetaData = UTATOutfitSettings::Get().OutfitMetadataTable;
   TWeakObjectPtr<ATATCharacter> weakThis(this);

   UAssetManager::GetStreamableManager().RequestAsyncLoad(outfitMetaData.ToSoftObjectPath(), [outfitLoadout, outfitMetaData, weakThis]
   {
      ATATCharacter* self = weakThis.Get();
      if (IsValid(self))
      {
         // Load only the 1P meshes if the character is locally controlled
         if (self->IsLocallyControlled())
         {
            if (const FTATCharacterLoadoutEntry* bodySlot = outfitLoadout.FindByPredicate([](const FTATCharacterLoadoutEntry& entry) {return entry.OutfitSlot == ETATCharacterOutfitSlot::Body; }))
            {
               if (const FTATOutfitsMetadataTableRow* metadata = UTATOutfitSettings::Get().FindOutfitMetadata(bodySlot->LoadoutTag, outfitMetaData.Get()))
               {
                  TSoftObjectPtr<USkeletalMesh> upperBodyMesh = metadata->OutfitSkeletalMesh1PUpperBody;
                  TSoftObjectPtr<USkeletalMesh> lowerBodyMesh = metadata->OutfitSkeletalMesh1PLowerBody;

                  TArray<FSoftObjectPath> pathsToLoad{ upperBodyMesh.ToSoftObjectPath(), lowerBodyMesh.ToSoftObjectPath() };

                  UAssetManager::GetStreamableManager().RequestAsyncLoad(MoveTemp(pathsToLoad), [weakThis, upperBodyMesh, lowerBodyMesh]
                     {
                        ATATCharacter* self = weakThis.Get();
                        if (IsValid(self))
                        {
                           self->Mesh1P_UpperBody->SetSkeletalMesh(upperBodyMesh.Get());
                           self->Mesh1P_LowerBody->SetSkeletalMesh(lowerBodyMesh.Get());
                        }
                     });
                  }
            }
         }
         
         
         // Load the 3P meshes for everyone
         TSoftObjectPtr<USkeletalMesh> headMesh;
         TSoftObjectPtr<USkeletalMesh> bodyMesh;

         for (const FTATCharacterLoadoutEntry& entry : outfitLoadout)
         {
            if (entry.OutfitSlot != ETATCharacterOutfitSlot::CallingCard)
            {
               if (const FTATOutfitsMetadataTableRow* metadata = UTATOutfitSettings::Get().FindOutfitMetadata(entry.LoadoutTag, outfitMetaData.Get()))
               {
                  if (entry.OutfitSlot == ETATCharacterOutfitSlot::Head)
                  {
                     headMesh = metadata->OutfitSkeletalMesh;
                  }
                  else if (entry.OutfitSlot == ETATCharacterOutfitSlot::Body)
                  {
                     bodyMesh = metadata->OutfitSkeletalMesh;
                  }
               }
            }
         }
         TArray<FSoftObjectPath> pathsToLoad{ headMesh.ToSoftObjectPath(), bodyMesh.ToSoftObjectPath() };
         UAssetManager::GetStreamableManager().RequestAsyncLoad(pathsToLoad, [weakThis, headMesh, bodyMesh]
            {
               ATATCharacter* self = weakThis.Get();
               if (IsValid(self))
               {
                  if (headMesh.IsValid())
                  {
                     self->Mesh3P_Head->SetSkeletalMeshAsset(headMesh.Get());
                  }

                  if (bodyMesh.IsValid())
                  {
                     self->Mesh3P_Body->SetSkeletalMeshAsset(bodyMesh.Get());
                  }
               }
            });
      }
   });
}

void ATATCharacter::_OnEquippedToolChanged()
{
   if (UToolComponent* tool = _GetCurrentTool())
   {
      TSubclassOf<UToolComponent> toolClass = tool->GetClass();
      if (toolClass != _recentlyEquippedWeaponClass)
      {
         if (UTATItemFunctionLibrary::IsWeaponTool(tool))
         {
            _recentlyEquippedWeaponClass = toolClass;
            OnRecentlyEquippedWeaponChanged.Broadcast(_recentlyEquippedWeaponClass);
            _RestartRecentlyUsedWeaponTimer();
         }
         else
         {
            _recentlyEquippedToolClass = toolClass;
            OnRecentlyEquippedToolChanged.Broadcast(_recentlyEquippedToolClass);
         }
      }
   }
}

void ATATCharacter::PossessedBy(AController* newController)
{
   Super::PossessedBy(newController);

   // Cache the team that our player state is on: this will allow us to remember what team we're on
   // even if we are unpossessed by the player controller (e.g. for astral projection)
   // N.B. this is not invalidated on UnPossessed deliberately so it can persist for that period
   const APlayerState* ps = GetPlayerState();
   HandleTeamChanged();

   if (const ATATPlayerState* tatPlayerState = Cast<ATATPlayerState>(ps))
   {
      tatPlayerState->GetLootInventoryComponent()->AuthoritySetToolset(GetToolSetInterface());

      // NOTE: Doing this here in both PossessedBy and OnRep_PlayerState
      //       Putting it in HandleSetPlayerState runs into ordering issues
      //       But this code will be removed anyways
      if (!bCreatedStaticItemInventoryComponent && tatPlayerState->GetTATItemInventory())
      {
         InitializeItemInventory(tatPlayerState->GetTATItemInventory());
      }

      _LoadAndApplyCharacterOutfit(tatPlayerState->GetCurrentOutfitLoadout());
   }
}

void ATATCharacter::NotifyControllerChanged()
{
   AController* oldController = PreviousController;

   Super::NotifyControllerChanged();

   AController* newController = Controller;

   if (oldController != nullptr)
   {
      UOSEInputSettings* inputSettings = UOSEInputSettings::GetOSEInputSettings();
      check(inputSettings);
      inputSettings->OnInputSettingsConfigChanged.RemoveAll(this);

      if (AOSEPlayerController* pc = Cast<AOSEPlayerController>(oldController))
      {
         pc->OnInputHardwareTypeChanged.RemoveAll(this);
      }
   }
   
   if(_stealthScoreComponent)
   {
      _stealthScoreComponent->OnStealthScoreChanged.RemoveAll(this);
   }
   
   if (AOSEPlayerController* pc = Cast<AOSEPlayerController>(newController))
   {
      pc->OnInputHardwareTypeChanged.AddUniqueDynamic(this, &ThisClass::_OnInputHardwareTypeChanged);

      UOSEInputSettings* inputSettings = UOSEInputSettings::GetOSEInputSettings();
      check(inputSettings);
      inputSettings->OnInputSettingsConfigChanged.AddUObject(this, &ThisClass::_OnInputSettingsConfigChanged);

      if(pc->IsLocalController() && _stealthScoreComponent)
      {
         _stealthScoreComponent->OnStealthScoreChanged.AddUObject(this, &ThisClass::_OnStealthScoreChanged);
      }
   }

   _RefreshSprintToggleSetting();

   if (_xrayComponent)
   {
      _xrayComponent->SetEnabled(!IsLocallyControlled());
   }
}

bool ATATCharacter::IsWeaponEquipped() const
{
   return UTATItemFunctionLibrary::DoesActorHaveWeaponEquipped(this);
}

UTATCombatComponent* ATATCharacter::GetTATCombatComponent() const
{
   return CastChecked<UTATCombatComponent>(GetCombatComponent());
}

void ATATCharacter::AuthorityOnKnockedOutByNonCharacterSource_Implementation()
{
   Super::AuthorityOnKnockedOutByNonCharacterSource_Implementation();

   const UTATProjectSettings& settings = *GetDefault<UTATProjectSettings>();
   const bool shouldBeDowned = settings.ShouldPlayerBeDownedByNonPlayerSources;

   if(shouldBeDowned == false)
   {
      _AuthorityResetAIKnowledgeOfMyself();
      _AuthorityDropAllInventory();
      _AuthorityRemoveRelevantThiefVisionIndicatorsOnKnockout();
   }
   
   // Check for a tool that wants to handle knockouts
   bool knockoutHandledByTool = false;
   _ForEachToolWithKnockoutHandlerInterface([&](UToolComponent* toolComponent)
   {
      if (ITATKnockoutHandlerInterface::Execute_OnCharacterKnockedOut(toolComponent, ETATKnockoutType::TemporarilyUnconscious, this, nullptr))
      {
         knockoutHandledByTool = true;
      }
   });

   if (knockoutHandledByTool)
   {
      return;
   }
   
   if(shouldBeDowned == false)
   {
      _AuthorityPermanentKO();
   }
   else
   {
      if (DownedTemporaryEffect != nullptr)
      {
         // We lost our health due to environmental or non-character damage, so enter a Downed state instead of a full KO
         UAbilitySystemComponent* asc = GetAbilitySystemComponent();
         check(asc);
         FGameplayEffectSpecHandle specHandle = asc->MakeOutgoingSpec(DownedTemporaryEffect, UGameplayEffect::INVALID_LEVEL, FGameplayEffectContextHandle());
         if (ensure(specHandle.IsValid()))
         {
            asc->ApplyGameplayEffectSpecToSelf(*specHandle.Data.Get());
         }
      }
      else
      {
         UE_LOG(LogTATCharacter, Error, TEXT("'%s': expected to have valid DownedTemporaryEffect"), *GetName());
      }
   }
}

void ATATCharacter::AuthorityOnEarlyDisconnect_Implementation()
{
}

void ATATCharacter::AuthorityOnKnockedOutByOtherCharacter_Implementation(AOSECharacterBase* otherCharacter)
{
   check(HasAuthority());

   Super::AuthorityOnKnockedOutByOtherCharacter_Implementation(otherCharacter);

   _AuthorityResetAIKnowledgeOfMyself();
   _AuthorityDropAllInventory();
   _AuthorityRemoveRelevantThiefVisionIndicatorsOnKnockout();

   // Check for a tool that wants to handle knockouts
   bool knockoutHandledByTool = false;
   _ForEachToolWithKnockoutHandlerInterface([&](UToolComponent* toolComponent)
   {
      if (ITATKnockoutHandlerInterface::Execute_OnCharacterKnockedOut(toolComponent, ETATKnockoutType::KnockedOutByOtherCharacter, this, otherCharacter))
      {
         knockoutHandledByTool = true;
      }
   });

   if (knockoutHandledByTool)
   {
      return;
   }

   _AuthorityPermanentKO();

   // Send a toast message to all players depending on what type of character knocked us out
   if (otherCharacter != nullptr)
   {
      if (ATATToastBroadcaster* toastBroadcaster = ATATToastBroadcaster::Get(this))
      {
         // If the other character is a player, broadcast a toast to all players
         if (APlayerState* otherPlayer = otherCharacter->GetPlayerState())
         {
            toastBroadcaster->ClientToastBroadcast_PlayerKnockedOutByPlayer(GetPlayerState(), otherPlayer);
         }
         else
         {
            // If the other character is not a player, only broadcast a toast if they were killed by a guard
            const uint8 otherCharacterTeam = otherCharacter->GetTeam();
            const bool ignoreDisguised = true;
            if (UTATTeamAttitudeSolver::TeamEquals(otherCharacterTeam, ETATTeamCharacterType::Guard, ignoreDisguised))
            {
               toastBroadcaster->ClientToastBroadcast_PlayerKnockedOutByGuard(GetPlayerState());
            }
         }
      }
   }

   if (ATATCharacter* otherTATCharacter = Cast<ATATCharacter>(otherCharacter))
   {
      // Give knockout credit to whoever knocked us out if they're a player
      if (ATATPlayerState* otherPS = otherTATCharacter->GetPlayerState<ATATPlayerState>())
      {
         if (ATATPlayerState* ourPS = GetPlayerState<ATATPlayerState>())
         {
            // Record that we were knocked out by another player
            ourPS->AuthorityWasKnockedOutByOtherPlayer(otherPS);
         }
         else
         {
            UE_LOG(LogTATCharacter, Warning, TEXT("Player '%s' was knocked out by another player but we could not find its player state"), *GetName());
         }
      }
      else
      {
         UE_LOG(LogTATCharacter, Warning, TEXT("Player '%s' knocked out by other player '%s', however couldn't get the player state"),
            *GetName(),
            *otherTATCharacter->GetName());
      }
   }
}

void ATATCharacter::AddObjectHighlight()
{
   int prevCount = _objectHighlightCount;
   _objectHighlightCount++;
   if (prevCount == 0 && _objectHighlightCount > 0)
   {
      OnObjectHighlightChanged.Broadcast(true);
   }
}

void ATATCharacter::RemoveObjectHighlight()
{
   int prevCount = _objectHighlightCount;
   _objectHighlightCount--;
   _objectHighlightCount = FMath::Max(_objectHighlightCount, 0);
   if (prevCount > 0 && _objectHighlightCount == 0)
   {
      OnObjectHighlightChanged.Broadcast(false);
   }
}

void ATATCharacter::HandleReactToOwnStim(const FGameplayTag& stimTag, float loudness)
{
   _stealthScoreComponent->HandleOwnStimReaction(stimTag, loudness);
}

void ATATCharacter::OnLyingDownChanged_Implementation(bool isLyingDown)
{
   Super::OnLyingDownChanged_Implementation(isLyingDown);
   if (HasAuthority() && isLyingDown == false)
   {
      // Ideally this would be done in the controller, however, because we re-enable collision overlaps on the line after
      // We can't use the event FOnLyingDownChanged. So we need to clear all of our overlapped respawn areas here.
      // So that when the collisions are re-enabled, only the correct areas are re-registered.
      if (ATATPlayerController* pc = GetController<ATATPlayerController>())
      {
         pc->AuthorityClearRespawnAreas();
      }
   }
   TATCollisionUtils::SetOverlayForLyingDown(this, isLyingDown);

   // go to third person when downed
   // NOTE: this will clobber any cheats that set the camera, but could add a flag if it is a problem
   if (AOSEPlayerController* pc = GetController<AOSEPlayerController>())
   {
      pc->SetPlayerCameraMode(isLyingDown ? EPlayerCameraMode::ThirdPerson : EPlayerCameraMode::Default);
   }

   // Prevent spinning the pawn while down
   // NOTE: assumes bUseControllerRotationYaw is true normally
   bUseControllerRotationYaw = !isLyingDown;
}

void ATATCharacter::OnUnconsciousChanged_Implementation(bool isUnconscious)
{
   Super::OnUnconsciousChanged_Implementation(isUnconscious);
   
   if (HasAuthority())
   {
      // Spawn a KO glyph on our body
      if (isUnconscious)
      {
         _AuthoritySpawnKOGlyph();
      }
      else
      {
         _AuthorityResetAIKnowledgeOfMyself();
      }
   }
   if (IsLocallyControlled())
   {
      if (UTATAnalyticsManager* analyticsManager = GetGameInstance()->GetSubsystem<UTATAnalyticsManager>())
      {
         FTATAnalyticsCustomFields fields;
         fields.Set(TEXT("Location"), GetActorLocation().ToCompactString());
         if (isUnconscious)
         {
            analyticsManager->OnDesignEventWithCustomFields(TEXT("PlayerKO"), fields);
         }
         else
         {
            analyticsManager->OnDesignEventWithCustomFields(TEXT("PlayerRevive"), fields);
         }
      }
   }

   // Remove any targeting freeze on KO or revive
   if (_interactionTargeter)
   {
      _interactionTargeter->ClearTargetingFreeze();
   }
}

bool ATATCharacter::IsBehavingSuspiciously(EAlertnessLevel allyAlertnessLevel) const
{
   // Bodies lying on the ground aren't behaving at all. There are other systems
   // for determing how AI reacts to bodies.
   if (IsUnconscious())
   {
      return false;
   }

   // Respecting the following matrix:
   // https://docs.google.com/spreadsheets/d/1GEleS17czuL0KKY45CqTkNNSIMu6EuJAUmzkmaXtudg

   UCharacterMovementComponent* movement = GetCharacterMovement();
   check(movement);

   const UTATProjectSettings& settings = *GetDefault<UTATProjectSettings>();

   if (IsCrouching() || movement->IsFalling())
   {
      // Is "IsFalling" right here? The spreadsheet specifies jumping
      if (allyAlertnessLevel >= EAlertnessLevel::Alerted)
      {
         return true;
      }
   }

   if (movement->IsMovingOnGround())
   {
      if (bIsSprinting)
      {
         // Running while allies are neutral/sus
         if (allyAlertnessLevel == EAlertnessLevel::Neutral || allyAlertnessLevel == EAlertnessLevel::Suspicious)
         {
            return true;
         }
      }
      else
      {
         // Walking around while allies are alerted
         if (allyAlertnessLevel >= EAlertnessLevel::Alerted)
         {
            return true;
         }
      }
   }
   else
   {
      // Standing around while allies are alerted
      if (allyAlertnessLevel >= EAlertnessLevel::Alerted)
      {
         return true;
      }
   }

   //Check for weapon out.
   bool hasWeaponEquipped = false;
   TScriptInterface<IToolSetInterface> toolSet = GetToolSetInterface();
   if (toolSet)
   {
      for (FGameplayTag weaponTag : settings.WeaponToolTags)
      {
         hasWeaponEquipped = toolSet->HasToolEquipped(weaponTag);
         if (hasWeaponEquipped)
         {
            break;
         }
      }
   }

   if (hasWeaponEquipped)
   {
      // Weapon out while allies are neutral/sus
      if (allyAlertnessLevel == EAlertnessLevel::Neutral || allyAlertnessLevel == EAlertnessLevel::Suspicious)
      {
         return true;
      }
   }
   else
   {
      // Weapon sheathed while allies are alerted
      if (allyAlertnessLevel >= EAlertnessLevel::Alerted)
      {
         return true;
      }
   }

   // Any other gameplay system that may mark the player as behaving suspiciously.
   // IE: lockpicking
   return HasMatchingGameplayTag(settings.SuspiciousActionStatusTag);
}

void ATATCharacter::AuthorityOnDetectionStateChanged(AActor* detector, const EActorDetectionState previousDetectionState, const EActorDetectionState currentDetectionState)
{
   check(HasAuthority());
   // If transitioning to "identified" state
   if (previousDetectionState  < currentDetectionState && currentDetectionState == EActorDetectionState::Identified)
   {
      const UTATProjectSettings& settings = UTATProjectSettings::Get();
      if (settings.PlayerDetectedStatTag.IsValid())
      {
         // Update "times detected" on player state...
         if (AOSEPlayerState* ps = GetPlayerState<AOSEPlayerState>())
         {
            ps->AuthorityUpdatePlayerStatInt(settings.PlayerDetectedStatTag, 1);
         }
      }
      else
      {
         UE_LOG(LogTATCharacter, Warning, TEXT("AuthorityOnDetectionStateChanged() failed to update Times Detected stat due to unassigned stat tag in UTATProjectSettings!"));
      }
   }
   const UTATAISettings& settings = UTATAISettings::Get();
   if(UAbilitySystemComponent* asc = GetAbilitySystemComponent())
   {
      if(UAbilitySystemComponent* detectorAsc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(detector))
      {
         // Find the gameplay effect for the previous detection state, then remove a single stack of it from the current actor
         // This step to remove the gameplay effect is to allow the de-escalation of identification
         if(const auto previousDetectionStateGameplayEffect = settings.DetectionStateToGameplayEffect.Find(previousDetectionState))
         {
            asc->RemoveActiveGameplayEffectBySourceEffect(*previousDetectionStateGameplayEffect, detectorAsc, 1);
         }
         // Find the gameplay effect for the new detection state, then add a single stack
         // The gameplay effects themselves are setup to automatically remove "lesser" detection states
         // Identified > Identifying > Observing
         if(const auto currentDetectionStateGameplayEffect = settings.DetectionStateToGameplayEffect.Find(currentDetectionState))
         {
            const FGameplayEffectSpecHandle spec = detectorAsc->MakeOutgoingSpec(*currentDetectionStateGameplayEffect, 0,
                                                                         detectorAsc->MakeEffectContext());
            detectorAsc->ApplyGameplayEffectSpecToTarget(*spec.Data.Get(), asc);
         }
      }
   }
}

void ATATCharacter::AuthorityOnDetectionValueUpdateForState(AActor* detector, EActorDetectionState detectionState,
   float detectionValue)
{
}

void ATATCharacter::_RefreshSprintToggleSetting()
{
   if (AOSEPlayerController* pc = GetController<AOSEPlayerController>())
   {
      if (pc->IsLocalController())
      {
         const EOSEInputHardwareType inputHardwareType = pc->GetCurrentInputHardwareType();

         auto* inputSettings = UOSEInputSettings::GetOSEInputSettings();
         check(inputSettings);
         bool newSprintIsToggled = inputSettings->IsSprintToggleSetForInputHardwareType(inputHardwareType);

         const EOSESprintInputBehavior newSprintBehavior = newSprintIsToggled ?
            EOSESprintInputBehavior::ContinueSprintAndAllowCancel :
            EOSESprintInputBehavior::SprintWhileRequested;

         if (newSprintBehavior != _sprintInputBehavior)
         {
            // If we're changing from a toggle-sprint to a hold-sprint, cancel a current sprint if we have one
            // to avoid it getting stuck on
            if (_HasAdditionalSprintCount())
            {
               SprintCancel();
            }

            _sprintInputBehavior = newSprintBehavior;
         }
      }
   }
}

void ATATCharacter::_OnInputSettingsConfigChanged()
{
   _RefreshSprintToggleSetting();
}

void ATATCharacter::_OnInputHardwareTypeChanged(EOSEInputHardwareType inputHardwareType)
{
   _RefreshSprintToggleSetting();
}

bool ATATCharacter::_ShouldShowOnMap() const
{
   if (const APlayerController* localController = GetWorld()->GetFirstPlayerController())
   {
      APawn* localPawn = localController->GetPawn();
      const APlayerState* localPlayerState = localController->GetPlayerState<APlayerState>();
      if (localPlayerState && localPawn == nullptr)
      {
         // The controller possesses the pawn _before_ setting the pawn value within the controller, so this can be null. 
         // HOWEVER, the player state keeps a reference to the pawn which is set before the possession occurs.
         localPawn = localPlayerState->GetPawn();
      }

      // Always show ourselves on the map
      if (localPawn == this)
      {
         return true;
      }

      // If this is another player...

      // First, check if we're friendly
      const EOSETeamAttitude attitude = UOSETeamFunctionLibrary::GetTeamAttitude(localPawn, this);
      // Always show friendly players on the map
      if (attitude == EOSETeamAttitude::Friendly)
      {
         return true;
      }

      // Check if Match Settings are configured to reveal us if we have Quest Loot
      if(const UTATMatchSettings* TATMatchSettings = UTATMatchSettingsBase::GetTATMatchSettings<UTATMatchSettings>(GetWorld()))
      {
         if (TATMatchSettings->DisplayAreasWithPlayersThatHaveQuestLoot)
         {
            // If we're not friendly, get the PlayerState of this character to check if we have Quest Loot
            if (const ATATPlayerState* thisPlayerState = GetPlayerState<ATATPlayerState>())
            {
               if (thisPlayerState->HasRelevantQuestLootInInventory())
               {
                  // Change our Map Actor Representation Data if we're being revealed due to having loot
                  _TogglePlayerHasQuestLootOnMap(true);
                  return true;
               }
               // If not, revert to normal Map Actor Representation Data
               _TogglePlayerHasQuestLootOnMap(false);
            }
         }
      }
   }

   return HasMatchingGameplayTag(ShowOnMapTag);
}

void ATATCharacter::_OnShowOnMapTagChanged(const FGameplayTag tag, const int32 newTagCount)
{
   const bool shouldShow = _ShouldShowOnMap();
   _mapActorComponent->SetActive(shouldShow);
}
