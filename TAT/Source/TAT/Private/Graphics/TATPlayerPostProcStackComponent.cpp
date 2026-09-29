// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Graphics/TATPlayerPostProcStackComponent.h"

// tat
#include "Graphics/TATPlayerPostProc.h"

// ose
#include "Character/OSECharacterBase.h"
#include "Player/OSEPlayerController.h"

// ue5
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/SpectatorPawn.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATPlayerPostProcStackComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATPlayerPostProcStackComponent, Log, All)

void UTATPlayerPostProcStackComponent::BeginPlay()
{
   Super::BeginPlay();

   if (AOSEPlayerController* controller = Cast<AOSEPlayerController>(GetOwner()))
   {
      // We only spawn and manage the post procs on local players, since they only affect graphics
      if (controller->IsLocalPlayerController())
      {
         _InitializeRelevantTags();
         _SpawnPostProcInstances();

         _currentlyPossessedPawn = controller->GetPawn();

         if (APawn* currentlyPossessedPawn = _currentlyPossessedPawn.Get())
         {
            _BindToTagChanges(currentlyPossessedPawn);
         }

         controller->OnPawnChanged.AddUniqueDynamic(this, &UTATPlayerPostProcStackComponent::_OnOwnerPawnChanged);
      }
   }
   else
   {
      UE_LOG(LogTATPlayerPostProcStackComponent, Warning,
         TEXT("UTATPlayerPostProcStackComponent component is on '%s' which is not a player controller"),
         *GetOwner()->GetName());
   }
}

void UTATPlayerPostProcStackComponent::EndPlay(const EEndPlayReason::Type reason)
{
   if (AOSEPlayerController* controller = Cast<AOSEPlayerController>(GetOwner()))
   {
      controller->OnPawnChanged.RemoveAll(this);
   }
   
   if (APawn* currentPawn = _currentlyPossessedPawn.Get())
   {
      _UnbindToTagChanges(currentPawn);
   }
   
   _DestroyPostProcInstances();

   Super::EndPlay(reason);
}

ATATPlayerPostProc* UTATPlayerPostProcStackComponent::GetPostProcInstanceByClass(TSubclassOf<ATATPlayerPostProc> postProcClass) const
{
   for (const FTATPlayerPostProcBinding& postProcBinding : PostProcBindings)
   {
      if (postProcBinding.PostProcClass == postProcClass)
      {
         return postProcBinding.PostProcInstance;
      }
   }

   return nullptr;
}

void UTATPlayerPostProcStackComponent::_OnOwnerPawnChanged(APawn* newPawn)
{
   if (APawn* previousPawn = _currentlyPossessedPawn.Get())
   {
      _UnbindToTagChanges(previousPawn);
   }

   for (const FTATPlayerPostProcBinding& postProcBinding : PostProcBindings)
   {
      if (IsValid(postProcBinding.PostProcInstance))
      {
         postProcBinding.PostProcInstance->SetVisibility(false);
      }
   }

   _currentlyPossessedPawn = newPawn;

   if (newPawn)
   {
      if (AOSECharacterBase* oseCharacter = Cast<AOSECharacterBase>(newPawn))
      {
         oseCharacter->CallOrRegisterAbilitiesInitializedDelegate(FSimpleMulticastDelegate::FDelegate::CreateUObject(this, &UTATPlayerPostProcStackComponent::_OnOwnerPawnAbilitiesInitialized));
      } else if(newPawn->IsA<ASpectatorPawn>() == false)
      {
         ensure(false);
      }
   }
}

void UTATPlayerPostProcStackComponent::_OnOwnerTagChanged(FGameplayTag tag, int32 count)
{
   _RefreshPostProcs();
}

void UTATPlayerPostProcStackComponent::_OnOwnerPawnAbilitiesInitialized()
{
   if (APawn* newPawn = _currentlyPossessedPawn.Get())
   {
      _RefreshPostProcs();
      _BindToTagChanges(newPawn);

      UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(newPawn);
      check(asc);

      // Notify each post-proc instance
      for (FTATPlayerPostProcBinding& postProcBinding : PostProcBindings)
      {
         if (IsValid(postProcBinding.PostProcInstance))
         {
            // TODO: amend this to prevent duplicate calls to OnAbilitiesReady() on repossession
            postProcBinding.PostProcInstance->OnAbilitiesReady(asc);
         }
      }
   }
}

void UTATPlayerPostProcStackComponent::_BindToTagChanges(APawn* newPawn)
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(newPawn))
   {
      for (FGameplayTag tag : _allRelevantTags)
      {
         FDelegateHandle handle = asc->RegisterGameplayTagEvent(tag).AddUObject(this, &UTATPlayerPostProcStackComponent::_OnOwnerTagChanged);
         _tagChangeBindings.Add(tag, handle);
      }
   }
}

void UTATPlayerPostProcStackComponent::_UnbindToTagChanges(APawn* newPawn)
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(newPawn))
   {
      for (TPair<FGameplayTag, FDelegateHandle> tagChangeBinding : _tagChangeBindings)
      {
         asc->UnregisterGameplayTagEvent(tagChangeBinding.Value, tagChangeBinding.Key);
      }
   }
}

void UTATPlayerPostProcStackComponent::_SpawnPostProcInstances()
{
   FActorSpawnParameters spawnParams;
   spawnParams.Owner = GetOwner();

   for (int32 i = 0; i < PostProcBindings.Num(); i++)
   {
      FTATPlayerPostProcBinding& postProcBinding = PostProcBindings[i];
      if (postProcBinding.PostProcClass)
      {
         postProcBinding.PostProcInstance = GetWorld()->SpawnActor<ATATPlayerPostProc>(postProcBinding.PostProcClass, spawnParams);
      }
      else
      {
         UE_LOG(LogTATPlayerPostProcStackComponent, Error,
            TEXT("Failed to spawn player post process for %s: PostProcBindings[%i] has an invalid PostProcClass (query='%s')"),
            *AActor::GetDebugName(GetOwner()), i, *postProcBinding.Query.GetDescription());
      }
   }
}

void UTATPlayerPostProcStackComponent::_DestroyPostProcInstances()
{
   for (FTATPlayerPostProcBinding& postProcBinding : PostProcBindings)
   {
      if (IsValid(postProcBinding.PostProcInstance))
      {
         postProcBinding.PostProcInstance->Destroy();
         postProcBinding.PostProcInstance = nullptr;
      }
   }
}

void UTATPlayerPostProcStackComponent::_RefreshPostProcs()
{
   if (UAbilitySystemComponent* asc = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(_currentlyPossessedPawn.Get()))
   {
      FGameplayTagContainer tagContainer;
      asc->GetOwnedGameplayTags(tagContainer);

      for (const FTATPlayerPostProcBinding& postProcBinding : PostProcBindings)
      {
         if (IsValid(postProcBinding.PostProcInstance))
         {
            const bool shouldBeEnabled = postProcBinding.Query.IsEmpty() || postProcBinding.Query.Matches(tagContainer);
            postProcBinding.PostProcInstance->SetVisibility(shouldBeEnabled);
         }
      }
   }
}

void UTATPlayerPostProcStackComponent::_InitializeRelevantTags()
{
   TArray<FGameplayTag> relevantTags;
   for (const FTATPlayerPostProcBinding& postProcBinding : PostProcBindings)
   {
      postProcBinding.Query.GetGameplayTagArray(relevantTags);

      for (FGameplayTag tag : relevantTags)
      {
         _allRelevantTags.Add(tag);
      }
   }
}
