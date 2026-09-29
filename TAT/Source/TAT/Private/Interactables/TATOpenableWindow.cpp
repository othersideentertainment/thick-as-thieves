// (c) 2018-2022 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Interactables/TATOpenableWindow.h"

// tat
#include "AI/SmartObjects/TATActionNodeComponent_IncorrectObjectState.h"
#include "AI/Target/TATTargetingGroups.h"
#include "Breakables/TATBreakableComponent.h"
#include "Developer/TATProjectSettings.h"
#include "Interactables/TATSecurityLockdownComponent.h"
#include "AI/Perception/TATPerceptionFunctionLibrary.h"

// ue5
#include "Engine/CollisionProfile.h"
#include "Logging/MessageLog.h"
#include "Misc/UObjectToken.h"
#include "Net/UnrealNetwork.h"
#include "Perception/AIPerceptionStimuliSourceComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATOpenableWindow)

UE_DEFINE_GAMEPLAY_TAG(TAG_AI_Object_Broken_Window, "AI.Object.Broken.Window")


ATATOpenableWindow::ATATOpenableWindow()
{
   _perceptionStimuliSource = CreateDefaultSubobject<UAIPerceptionStimuliSourceComponent>(TEXT("PerceptionStimuliSourceComponent"));
   
   USceneComponent* sceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
   RootComponent = sceneComponent;
   RootComponent->SetMobility(EComponentMobility::Static);
   
   _incorrectStateActionNodeComponent = CreateDefaultSubobject<UTATActionNodeComponent_IncorrectObjectState>(TEXT("IncorrectStateActionNode"));
   _incorrectStateActionNodeComponent->SetupAttachment(RootComponent);
   _incorrectStateActionNodeComponent->Mobility = EComponentMobility::Static;
   
   _securityLockdownComponent = CreateDefaultSubobject<UTATSecurityLockdownComponent>(TEXT("SecurityLockdown"));
   _breakableComponent->SetIsBreakableByDefault(true);
}

void ATATOpenableWindow::GetLifetimeReplicatedProps(TArray< FLifetimeProperty >& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME(ATATOpenableWindow, _lockedDown);
}

FGameplayTag ATATOpenableWindow::GetUtilityAITargetingGroup() const
{
   if(_breakableComponent && _breakableComponent->IsBroken())
   {
      return TAG_AI_TargetingGroup_SmartObject_BrokenObject;
   }
   return TAG_AI_TargetingGroup_SmartObject_IncorrectState;
}

UTATSmartObjectComponent* ATATOpenableWindow::GetSmartObjectComponent() const
{
   return _incorrectStateActionNodeComponent;
}

void ATATOpenableWindow::PostInitializeComponents()
{
   Super::PostInitializeComponents();

   if (HasAuthority() && _incorrectStateActionNodeComponent != nullptr)
   {
      // Incorrect state component will pay attention to whether this actor is broken or not.
      // If broken, the object will be marked as not in an incorrect state as it's a state 
      // the AI can't do anything about
      _incorrectStateActionNodeComponent->AssignBreakableComponent(_breakableComponent);

      // Incorrect state component will enable/disable the stimuli source component based on
      // whether the object is in a correct/incorrect state. If in incorrect state, it will be 
      // visible. If in a correct state, it will not be visible.
      _incorrectStateActionNodeComponent->AssignStimuliSourceComponent(_perceptionStimuliSource);

      const int32 currentState = IsOn() ? 1 : 0;
      _incorrectStateActionNodeComponent->SetInitialState(currentState, currentState);
   }
}

void ATATOpenableWindow::BeginPlay()
{
   Super::BeginPlay();
   
   _securityLockdownComponent->OnSecurityLockdownStateChanged.AddUniqueDynamic(this, &ThisClass::_OnSecurityLockdownStateChanged);
}

bool ATATOpenableWindow::AuthorityIsObjectInCorrectState_Implementation(bool allowIgnoringOfState) const
{
   check(_incorrectStateActionNodeComponent != nullptr);
   return _incorrectStateActionNodeComponent->IsStateCorrect(allowIgnoringOfState);
}

FTATLockableToggleAllowedDirections ATATOpenableWindow::_GetAllowedDirections() const
{
   return _lockedDown && _overrideLockdownAllowedDirections ? _lockdownAllowedDirections : _normalAllowedDirections;
}

bool ATATOpenableWindow::CanBeSeenFrom(const FVector& observerLocation, FVector& outSeenLocation, int32& numberOfLoSChecksPerformed, float& outSightStrength, const AActor* ignoreActor, const bool* wasVisible, int32* userData) const
{
   static constexpr bool kNonColliding = false;
   static constexpr bool kIncludeFromChildActors = false;
   const FBox actorBounds = GetComponentsBoundingBox(kNonColliding, kIncludeFromChildActors);
   return UTATPerceptionFunctionLibrary::CanActorBoundingBoxBeSeenFromLocation(
      this,
      actorBounds,
      observerLocation,
      outSeenLocation,
      numberOfLoSChecksPerformed,
      outSightStrength,
      ignoreActor
   );
}

bool ATATOpenableWindow::DoesSupportInteractionBy_Implementation(ACharacter* interactor, const FGameplayTag& interactorIdentity) const
{
   return _allowedInteractors.HasTag(interactorIdentity);
}

#if WITH_EDITOR
void ATATOpenableWindow::CheckForErrors()
{
   Super::CheckForErrors();

   TArray<UActorComponent*> primitiveComponents;
   GetComponents(UPrimitiveComponent::StaticClass(), primitiveComponents);

   TArray<FCollisionProfileName> allowedCollisionProfiles = UTATProjectSettings::Get().AllowedCollisionProfilesForOpenableDoorsAndWindows;

   // We are looking to make sure at least one primitive component has explicitly allowed the astral projection pawn,
   // otherwise it's likely we need to add it to the ones that need it. For that, NoCollision doesn't count
   allowedCollisionProfiles.RemoveAll([&](const FCollisionProfileName& item)
   {
      static const FName kNoCollisionProfileName(TEXT("NoCollision"));
      return item.Name == kNoCollisionProfileName;
   });

   FString allowedCollisonProfileNamesString = TEXT("(");
   for (const FCollisionProfileName& allowedCollisionProfile : allowedCollisionProfiles)
   {
      allowedCollisonProfileNamesString += allowedCollisionProfile.Name.ToString();
      allowedCollisonProfileNamesString += TEXT(",");
   }
   allowedCollisonProfileNamesString += TEXT(")");

   FMessageLog msgLog(FName("MapCheck"));

   int32 numPrimitiveComponentsWithCollisionProfile = 0;
   for (UActorComponent* component : primitiveComponents)
   {
      if (auto* primitiveComp = Cast<UPrimitiveComponent>(component))
      {
         const FName collisionProfileName = primitiveComp->GetCollisionProfileName();
         const bool isCollisonProfileAllowed = allowedCollisionProfiles.ContainsByPredicate([&](const FCollisionProfileName& item)
         {
            return item.Name == collisionProfileName;
         });

         if (isCollisonProfileAllowed)
         {
            numPrimitiveComponentsWithCollisionProfile++;
         }
         else
         {
            // If a component has the Glass tag, it's part of the actual window geometry: that means it should have the correct collision profile
            // to allow the astral projection character to pass through it
            static const FName kGlassComponentTag(TEXT("Glass"));
            if (primitiveComp->ComponentHasTag(kGlassComponentTag))
            {
               msgLog.Error()
                  ->AddToken(FUObjectToken::Create(this))
                  ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
                     TEXT("ATATOpenableWindow '%s' (BP: '%s') has a primitive component with the tag '%s', but with an unexpected collision profile '%s'. ")
                     TEXT("If it's part of the actual window, it should have a collision profile that's one of: %s."),
                     *GetName(),
                     *GetClass()->GetName(),
                     *kGlassComponentTag.ToString(),
                     *collisionProfileName.ToString(),
                     *allowedCollisonProfileNamesString))));
            }
         }
      }
   }

   // As an additional fallback check: if we have any primitive components, but none of them specifically allow the astral projection pawn,
   // then trigger an error because it's very likely that we need to go through and mark the window geometry with the correct collision profile
   if (primitiveComponents.Num() > 0 && numPrimitiveComponentsWithCollisionProfile == 0)
   {
      msgLog.Error()
         ->AddToken(FUObjectToken::Create(this))
         ->AddToken(FTextToken::Create(FText::FromString(FString::Printf(
            TEXT("ATATOpenableWindow '%s' (BP: '%s') has no primitive components with the expected collision profile. ")
            TEXT("At least one should have a collision profile that's one of: %s."),
            *GetName(),
            *GetClass()->GetName(),
            *allowedCollisonProfileNamesString))));
   }
}
#endif

FGameplayTagCountContainer& ATATOpenableWindow::GetGameplayTagCountContainer()
{
   check(_incorrectStateActionNodeComponent != nullptr);
   return _incorrectStateActionNodeComponent->GetIncorrectStateTags();
}

void ATATOpenableWindow::_OnStateChanged(bool bIsOn, bool bWasRecent)
{
   Super::_OnStateChanged(bIsOn, bWasRecent);

   check(_incorrectStateActionNodeComponent != nullptr);
   const int32 currentState = IsOn() ? 1 : 0;
   _incorrectStateActionNodeComponent->SetCurrentState(currentState);
}

void ATATOpenableWindow::_OnSecurityLockdownStateChanged(bool bIsInLockdown)
{
   if(HasAuthority() && bIsInLockdown)
   {
      if(IsOn())
      {
         SetOn(false);
      }
      // Lockdown always locks on both sides
      SetLockDirection(EToggleLockDirection::Both);
      SetLocked(true);

      if(!_lockedDown)
      {
         FlushNetDormancy();
         _lockedDown = true;
      }
   }
   UpdateLockdownVisual(bIsInLockdown);
}
