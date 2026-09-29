// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Targeting/TATStateTreeTargetingTypes.h"

// tat
#include "AI/TATAIController.h"
#include "AI/TATKnowledgeComponent.h"
#include "Character/TATCharacterAIBase.h"
#include "Environment/TATPrivateSpaceCharacterComponent.h"
#include "Loot/TATLootInterface.h"
#include "Loot/TATLootInventory.h"
#include "Variation/Clues/TATNPCClueComponent.h"
#include "AI/Navigation/TATNavLinkOwnerInterface.h"
#include "AI/SmartObjects/TATSmartObjectComponent.h"
#include "AI/SmartObjects/TATSmartObjectOwnerInterface.h"
#include "AI/Utility/TATUtilityAITargetingGroupInterface.h"

// ose
#include "OSEIndividualKnowledgeInterface.h"
#include "AI/OSEAIController.h"
#include "Character/OSECharacterBase.h"
#include "AI/OSEAISettings.h"
#include "AI/Utility/ResponseCurve.h"

// ue
#include "Kismet/KismetMathLibrary.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeTargetingTypes)

#if WITH_EDITOR
void FTATStateTreeTargetingConsideration::RefreshTitleProperty()
{
#if WITH_EDITORONLY_DATA
   TitlePropertyHidden = FString::Printf(TEXT("%s%s")
      , Enabled ? TEXT("") : TEXT("[Disabled] ")
      , *Name.ToString());
#endif // WITH_EDITORONLY_DATA
}

void UTATStateTreeTargetingConsiderations::_RefreshTitleProperty()
{
   for (FTATStateTreeTargetingConsideration& consideration : Considerations)
   {
      consideration.RefreshTitleProperty();
   }
}

void UTATStateTreeTargetingConsiderations::PostLoad()
{
   Super::PostLoad();
   _RefreshTitleProperty();
}

void UTATStateTreeTargetingConsiderations::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);
   _RefreshTitleProperty();
}

void UTATStateTreeTargetingConsiderations::PostEditUndo()
{
   Super::PostEditUndo();
   _RefreshTitleProperty();
}

EDataValidationResult UTATStateTreeTargetingConsiderations::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   for (int i = 0; i < Considerations.Num(); i++)
   {
      const FTATStateTreeTargetingConsideration& consideration = Considerations[i];
      const UTATStateTreeTargetingConsiderationInput* input = consideration.Input;
      if (input == nullptr)
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("\"%s\" input at index %d does not have an Input!"),
            *consideration.Name.ToString(), i)));
      }
      else if (!input->DoesSupportTargetType(TargetType))
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("\"%s\" (%s) input at index %d does not support '%s' target type!"),
            *consideration.Name.ToString(), *input->GetName(), i, *UEnum::GetValueAsString(TargetType))));
      }
   }

   return context.GetIssues().IsEmpty() ? result : EDataValidationResult::Invalid;
}
#endif // WITH_EDITOR

float UTATStateTreeTargetingConsiderationInput::GetValue(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   check(IsTargetContextValid(targetContext));
   return GetValueInternal(aiController, character, targetContext);
}

bool UTATStateTreeTargetingConsiderationInput::IsTargetContextValid(const FTargetContext& targetContext) const
{
   const ETargetType supported = GetSupportedTargetTypeBitmask();
   if (supported == ETargetType::ALL)
   {
      return targetContext.IsValidForTargetType(ETargetType::ALL);
   }
   if (EnumHasAnyFlags(supported, ETargetType::KnownActors))
   {
      if (!targetContext.IsValidForTargetType(ETargetType::KnownActors))
      {
         return false;
      }
   }
   if (EnumHasAnyFlags(supported, ETargetType::Players))
   {
      if (!targetContext.IsValidForTargetType(ETargetType::Players))
      {
         return false;
      }
   }
   return true;
}

float UTATStateTreeTargetingConsiderationInput_Composite::GetValueInternal(
   AOSEAIController* aiController,
   AOSECharacterBase* character,
   const FTargetContext& targetContext) const
{
   TArray<float, TInlineAllocator<8>> results;
   results.Reserve(Considerations.Num());

   for(const auto& consideration : Considerations)
   {
      if (!consideration.Input)
         continue;

      if (!consideration.Enabled)
         continue;

      const float inputValue = consideration.Input->GetValue(aiController, character, targetContext);
      const float inputScore = consideration.ResponseCurve ? consideration.ResponseCurve->GetFloatValue(inputValue) : inputValue;
      results.Emplace(inputScore);
   }

   return _GetCompositeValue(results);
}

float UTATStateTreeTargetingConsiderationInput_Composite_Max::_GetCompositeValue(const TArrayView<float>& results) const
{
   float maxResult = 0.0f;
   for(const float& result : results)
   {
      maxResult = FMath::Max(maxResult, result);
   }
   return maxResult;
}

float UTATStateTreeTargetingConsiderationInput_Composite_NonZero::_GetCompositeValue(const TArrayView<float>& results) const
{
   if (results.Num() == 0.0f)
      return 1.0f;

   const UOSEAISettings& settings = UOSEAISettings::Get();
   for (const float& result : results)
   {
      if (result < settings.MinStateScore)
         return 0.0f;
   }

   return 1.0f;   
}

float UTATStateTreeTargetingConsiderationInput_IsAttitude::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   return targetContext.GetKnowledge().GetAttitude() == TargetAttitude ? 1.f : 0.f;
}

float UTATStateTreeTargetingConsiderationInput_MatchesVisibility::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   return targetContext.GetKnowledge().GetIsVisible() == TargetVisibility ? 1.f : 0.f;
}

float UTATStateTreeTargetingConsiderationInput_Distance::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   const float distance = character->GetDistanceTo(targetContext.Actor);
   return CalculateNormalizedValue(distance);
}

float UTATStateTreeTargetingConsiderationInput_Distance::CalculateNormalizedValue(float distance) const
{
   if(distance < MinDistance)
   return 0.f;
   if(distance > MaxDistance)
      return 0.f;
   return UKismetMathLibrary::NormalizeToRange(distance, MinDistance, MaxDistance);
}

float UTATStateTreeTargetingConsiderationInput_PathDistance::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   return CalculateNormalizedValue(targetContext.GetKnowledge().GetPathLengthToLastKnownLocation());
}

float UTATStateTreeTargetingConsiderationInput_MatchesTagQuery::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   const IGameplayTagAssetInterface* tagAssetInterface = Cast<IGameplayTagAssetInterface>(targetContext.Actor);
   if(tagAssetInterface == nullptr)
      return 0.f;
   FGameplayTagContainer tagContainer;
   tagAssetInterface->GetOwnedGameplayTags(tagContainer);
   return TagQuery.Matches(tagContainer) ? 1.f : 0.f;
}

float UTATStateTreeTargetingConsiderationInput_HasIndividualKnowledgeOfTarget::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   const IOSEIndividualKnowledgeInterface* individualKnowledgeInterface = Cast<IOSEIndividualKnowledgeInterface>(aiController);
   if(const UOSEIndividualKnowledgeComponent* individualKnowledgeComponent = individualKnowledgeInterface->GetIndividualKnowledgeComponent())
   {
      const AActor* target = targetContext.Actor;
      const FIndividualKnowledge* individualKnowledge = individualKnowledgeComponent->FindKnowledgeForActor(target);
      if(individualKnowledge == nullptr)
      {
         return 0.f;
      }
      return QueryToRun.Matches(individualKnowledge->GameplayTagContainer) ? 1.f : 0.f;
   }
   return 0.f;
}

float UTATStateTreeTargetingConsiderationInput_HasMajorLoot::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   if(character == nullptr)
   return 0.f;

   const ITATLootInventoryInterface* inventoryInterface = Cast<ITATLootInventoryInterface>(character);
   if(inventoryInterface == nullptr)
      return 0.f;

   const UTATLootInventoryComponent* lootInventoryComponent = inventoryInterface->GetLootInventoryComponent();
   if(lootInventoryComponent == nullptr)
      return 0.f;

   return lootInventoryComponent->HasMajorLoot() ? 1.f : 0.f;
}

float UTATStateTreeTargetingConsiderationInput_HasTrait::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   const ATATCharacterAIBase* targetCharacter = Cast<ATATCharacterAIBase>(targetContext.Actor);
   if(targetCharacter == nullptr)
      return 0.f;
      
   const ATATAIController* targetController = Cast<ATATAIController>(targetCharacter->GetController());
   if (targetController == nullptr)
      return 0.0f;

   if(targetController->HasTrait(Trait) == false)
      return 0.f;

   return 1.f;
}

float UTATStateTreeTargetingConsiderationInput_HasPathToTarget::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   const bool foundPath = RequireFullPath ? targetContext.GetKnowledge().GetHasFullPathToLastKnownLocation() : targetContext.GetKnowledge().GetHasAnyPathToLastKnownLocation();
   return foundPath ? 1.f : 0.f;
}

float UTATStateTreeTargetingConsiderationInput_CheckNumberOfNavAgents::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   const AActor* actor = targetContext.Actor;
   if(actor == nullptr)
      return 0.f;

   if (!actor->Implements<UTATNavLinkOwnerInterface>())
      return ShouldFailIfTargetNotNavLinkOwner ? 0.0f : 1.0f;

   const UTATNavLinkOwnerComponent* navLinkOwnerComponent = ITATNavLinkOwnerInterface::Execute_GetNavLinkOwnerComponent(actor);
   if(navLinkOwnerComponent == nullptr)
      return 0.f;

   const int countOfAgents = navLinkOwnerComponent->GetNumberOfAgentsReservingNavLink();
   return UOSEMathFunctionLibrary::CompareInts(
      countOfAgents,
      NumberOfAgents,
      ComparisonMethod) ? 1.f : 0.f;
}

float UTATStateTreeTargetingConsiderationInput_AlertnessMatches::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   const AOSECharacterBase* characterBase = Cast<AOSECharacterBase>(targetContext.Actor);
   if(characterBase == nullptr)
   {
      return 0.f;
   }
   const ATATAIController* otherCharacterController = Cast<ATATAIController>(characterBase->GetController());
   if (otherCharacterController == nullptr)
   {
      return 0.f;
   }
   return UOSEMathFunctionLibrary::CompareInts(
      static_cast<int>(otherCharacterController->GetAlertnessLevel()),
      static_cast<int>(AlertnessLevel),
      ComparisonMethod) ? 1.f : 0.f;
}

float UTATStateTreeTargetingConsiderationInput_DetectionMatches::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   return UOSEMathFunctionLibrary::CompareInts(
      static_cast<int>(targetContext.GetKnowledge().GetDetectionState()),
      static_cast<int>(DetectionLevel),
      ComparisonMethod) ? 1.f : 0.f;
}

float UTATStateTreeTargetingConsiderationInput_KnowledgeSourceMatches::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   return (targetContext.GetKnowledge().GetKnowledgeSource() == KnowledgeSource);
}

float UTATStateTreeTargetingConsiderationInput_IsInAccessibleZone::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
	AActor* targetActor = targetContext.Actor;
	if (targetActor == nullptr)
	{
		return 0.0f;
	}
	const UTATPrivateSpaceCharacterComponent* targetPrivateZoneComponent = UTATPrivateSpaceCharacterComponent::TryGet(targetActor);
	if (targetPrivateZoneComponent == nullptr)
	{
		// If the target actor doesn't have any allowed private zones, then presumably any AI
		// should be able to navigate to them.
		// ASSUMPTION : the target actor isn't in a place they're not supposed to be.
		return 1.0f;
	}

	const FGameplayTagContainer& targetAllowedZones = targetPrivateZoneComponent->AuthorityGetAllAllowedPrivateZone();
	if (targetAllowedZones.IsEmpty())
	{
		// If the target actor doesn't have any allowed private zones, then presumably any AI
		// should be able to navigate to them.
		// ASSUMPTION : the target actor isn't in a place they're not supposed to be.
		return 1.0f;
	}

	if (const UTATPrivateSpaceCharacterComponent* selfPrivateZoneComponent = UTATPrivateSpaceCharacterComponent::TryGet(character))
	{
		const FGameplayTagContainer& selfAllowedZones = selfPrivateZoneComponent->AuthorityGetAllAllowedPrivateZone();
		const bool isSelfAllowed = selfAllowedZones.HasAll(targetAllowedZones);
		return isSelfAllowed ? 1.0f : 0.0f;
	}

	// Target actor has allowed private zones but we have none. 
	return 0.0f;
}

float UTATStateTreeTargetingConsiderationInput_IsClueInteractableByTarget::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   const ATATCharacterAIBase* tatCharacter = Cast<ATATCharacterAIBase>(character);
   if (tatCharacter == nullptr)
   {
      return 0.0f;
   }

   const UTATNPCClueComponent* clueComponent = tatCharacter->GetNPCClueComponent();
   if (clueComponent == nullptr)
   {
      return 0.0f;
   }

   const APawn* targetPawn = Cast<APawn>(targetContext.Actor);
   if (targetPawn == nullptr)
   {
      return 0.0f;
   }

   const APlayerState* playerState = targetPawn->GetPlayerState();
   if (playerState == nullptr)
   {
      return 0.0f;
   }

   return clueComponent->HasClueToShareWithPlayer(playerState) ? 1.0f : 0.0f;
}

float UTATStateTreeTargetingConsiderationInput_LineTraceToTarget::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
	const ATATCharacterAIBase* tatCharacter = Cast<ATATCharacterAIBase>(character);
	if (tatCharacter == nullptr)
	{
		return 0.0f;
	}

	UTATAsyncRequestComponent* asyncRequestComponent = tatCharacter->GetAsyncRequestComponent();
	if (asyncRequestComponent == nullptr)
	{
		return 0.0f;
	}

	FTATAsyncTraceRequestContext request;
	request.Source = character;
	request.SourceLocation = SourceLocation;
	request.Target = targetContext.Actor;
	request.TargetLocation = TargetLocation;
	request.CollisionProfile = CollisionProfile;

	const UTATAsyncRequestComponent::FAsyncRequestDataCache& data = asyncRequestComponent->RequestAsyncLineTraceData(request);
	if (!data.HasData())
	{
		// Most likely first trace request that hasn't finished yet.
		return 0.0f;
	}

	const bool foundBlockingHit = data.HitResult.IsSet() && data.HitResult->IsValidBlockingHit();
	return (foundBlockingHit != SucceedIfUnblocked) ? 1.0f : 0.0f;
}

float UTATStateTreeTargetingConsiderationInput_CanInteractWithTarget::GetValueInternal(AOSEAIController* aiController, AOSECharacterBase* character, const FTargetContext& targetContext) const
{
   const ATATCharacterAIBase* tatCharacter = Cast<ATATCharacterAIBase>(character);
   if (tatCharacter == nullptr)
   {
      return 0.0f;
   }
   if (targetContext.Actor && 
      targetContext.Actor->Implements<UInteractableInterface>() && 
      IInteractableInterface::Execute_IsInteractable(targetContext.Actor, character))
   {
      return 1.f;
   }
   
   return 0.f;
}

float UTATStateTreeTargetingConsiderationInput_CheckUtilityTargetingGroup::GetValueInternal(AOSEAIController* aiController,
   AOSECharacterBase* character,
   const FTargetContext& targetContext) const
{
   const ITATUtilityAITargetingGroupInterface* targetingGroupInterface = Cast<ITATUtilityAITargetingGroupInterface>(targetContext.Actor);
   if(targetingGroupInterface == nullptr)
   {
      return 0.f;
   }

   return TagQuery.Matches(FGameplayTagContainer(targetingGroupInterface->GetUtilityAITargetingGroup())) ? 1.f : 0.f;
}

float UTATStateTreeTargetingConsiderationInput_SmartObjectHasSlotAvailable::GetValueInternal(AOSEAIController* aiController,
   AOSECharacterBase* character,
   const FTargetContext& targetContext) const
{
   const ITATSmartObjectOwnerInterface* smartObjectOwnerInterface = Cast<ITATSmartObjectOwnerInterface>(targetContext.Actor);
   if(smartObjectOwnerInterface == nullptr)
      return 0.f;

   const USmartObjectSubsystem* smartObjectSubsystem = USmartObjectSubsystem::GetCurrent(character->GetWorld());
   if(smartObjectSubsystem == nullptr)
      return 0.f;

   const UTATSmartObjectComponent* smartObjectComponent = smartObjectOwnerInterface->GetSmartObjectComponent();
   if(smartObjectComponent == nullptr)
      return 0.f;
   
   const FSmartObjectActorUserData actorUserData(character);
   const FConstStructView actorUserDataView(FConstStructView::Make(actorUserData));
   
   TArray<FSmartObjectSlotHandle> outSlots;
   smartObjectSubsystem->FindSlots(smartObjectComponent->GetRegisteredHandle(), Filter, outSlots, actorUserDataView);
   for (const FSmartObjectSlotHandle& slot : outSlots)
   {
      if(smartObjectSubsystem->GetSlotState(slot) == ESmartObjectSlotState::Free)
         return 1.f;
   }
   return 0.f;
}
