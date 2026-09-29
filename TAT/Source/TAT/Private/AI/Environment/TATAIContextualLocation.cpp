// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Environment/TATAIContextualLocation.h"

// tat
#include "AI/Environment/TATAIContextualLocationSubsystem.h"
#include "Character/TATCharacterAIBase.h"

// ue
#include "Components/ArrowComponent.h"
#include "Components/CapsuleComponent.h"

#if WITH_EDITOR
#include "Misc/DataValidation.h"
#endif // WITH_EDITOR

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATAIContextualLocation)

ATATAIContextualLocation::ATATAIContextualLocation(const FObjectInitializer& objectInitializer) : Super(objectInitializer)
{
   PrimaryActorTick.bCanEverTick = false;
   bNetLoadOnClient = false;
   bGenerateOverlapEventsDuringLevelStreaming = true;
   if (UCapsuleComponent* capsuleComponent = GetCapsuleComponent())
   {
      capsuleComponent->SetCollisionProfileName(FName(TEXT("Trigger")));
      capsuleComponent->InitCapsuleSize(40.0f, 92.0f);
#if WITH_EDITORONLY_DATA
      _arrowComponent = CreateEditorOnlyDefaultSubobject<UArrowComponent>(TEXT("Arrow"));

      if (_arrowComponent)
      {
         _arrowComponent->ArrowColor = FColor(235, 64, 52);
         _arrowComponent->ArrowSize = 1.0f;
         _arrowComponent->bTreatAsASprite = true;
         _arrowComponent->SetupAttachment(capsuleComponent);
         _arrowComponent->bIsScreenSizeScaled = true;
      }
#endif // WITH_EDITORONLY_DATA
   }
}

#if WITH_EDITOR
EDataValidationResult ATATAIContextualLocation::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   const FGameplayTag type = GetLocationType();
   if (!type.IsValid())
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("[%s] has unassigned/invalid point type tag!"), *GetName())));
      result = EDataValidationResult::Invalid;
   }

   return result;
}
#endif // WITH_EDITOR

void ATATAIContextualLocation::BeginPlay()
{
   Super::BeginPlay();

   if (UTATAIContextualLocationSubsystem* contextualLocationSubsystem = GetWorld()->GetSubsystem<UTATAIContextualLocationSubsystem>())
   {
      contextualLocationSubsystem->RegisterLocation(this);
   }
}

void ATATAIContextualLocation::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (UTATAIContextualLocationSubsystem* contextualLocationSubsystem = GetWorld()->GetSubsystem<UTATAIContextualLocationSubsystem>())
   {
      contextualLocationSubsystem->UnregisterLocation(this);
   }

   Super::EndPlay(endPlayReason);
}

bool ATATAIContextualLocation::TryClaim(ATATCharacterAIBase* aiCharacter)
{
   if (!IsClaimed() && aiCharacter != nullptr)
   {
      aiCharacter->OnDestroyed.AddDynamic(this, &ATATAIContextualLocation::_OnClaimingAIDestroyed);

      _claimedByAICharacter = aiCharacter;
      return true;
   }

   return false;
}

bool ATATAIContextualLocation::TryRelease(ATATCharacterAIBase* aiCharacter)
{
   if (IsClaimed() && _claimedByAICharacter == aiCharacter)
   {
      bool isInUseStateChanged = false;
      verify(SetIsInUse(false, aiCharacter, isInUseStateChanged));

      aiCharacter->OnDestroyed.RemoveDynamic(this, &ATATAIContextualLocation::_OnClaimingAIDestroyed);

      _claimedByAICharacter = nullptr;
      return true;
   }

   return false;
}

bool ATATAIContextualLocation::SetIsInUse(bool isInUse, const ATATCharacterAIBase* usingAICharacter, bool& isInUseStateChanged)
{
   if (!ensureMsgf(usingAICharacter != nullptr, TEXT("%s had SetIsInUse called with a null usingAICharacter."),
      *GetName()))
   {
      return false;
   }

   if (!ensureMsgf(_claimedByAICharacter == usingAICharacter, TEXT("%s was attempted to be used by %s while claimed by %s."),
      *GetName(), *GetNameSafe(usingAICharacter), *GetNameSafe(_claimedByAICharacter.Get())))
   {
      return false;
   }

   isInUseStateChanged = (_isInUse != isInUse);
   if (isInUseStateChanged)
   {
      _isInUse = isInUse;
      OnIsInUseStateChanged(_isInUse);
   }

   // Contextual location 'isInUse' was set to the asked-for state.
   // Whether the state actually changed is tracked with 'isInUseStateChanged'.
   return true;
}

void ATATAIContextualLocation::_OnClaimingAIDestroyed(AActor* actor)
{
   verify(TryRelease(CastChecked<ATATCharacterAIBase>(actor)));
}
