// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "AI/StateTrees/Distractions/TATStateTreeDistractionComponent.h"

// tat
#include "AI/TATAIController.h"
#include "AI/StateTrees/TATStateTreeEvents.h"

// ue
#include "EnvQueryItemType_SmartObject.h"
#include "EnvironmentQuery/EnvQueryManager.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATStateTreeDistractionComponent)

UTATStateTreeDistractionComponent::UTATStateTreeDistractionComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
}

void UTATStateTreeDistractionComponent::BeginPlay()
{
   Super::BeginPlay();
   
   _TATAIController = CastChecked<ATATAIController>(GetOwner());
   _StateTreeAIComponent = _TATAIController->FindComponentByClass<UTATStateTreeAIComponent>();
}

void UTATStateTreeDistractionComponent::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   StopDelayTimer();
   Super::EndPlay(endPlayReason);
}

void UTATStateTreeDistractionComponent::OnDistractionPossible()
{
   if(_IsDistractionAllowed == false)
      return;
   if(_InProgressRequestID != INDEX_NONE)
      return;
   
   FEnvQueryRequest request(_QueryTemplate, GetOwner());
   _InProgressRequestID = request.Execute(_EQSRunMode, FQueryFinishedSignature::CreateUObject(this, &ThisClass::OnDistractionEQSResult));
}

void UTATStateTreeDistractionComponent::OnDistractionEQSResult(TSharedPtr<FEnvQueryResult> envQueryResult)
{
   _InProgressRequestID = INDEX_NONE;
   if(_IsDistractionAllowed == false)
      return;
   if(envQueryResult == nullptr)
      return;
   if(envQueryResult->IsSuccessful() == false)
      return;
   if (envQueryResult->ItemType->IsChildOf(UEnvQueryItemType_SmartObject::StaticClass()) == false)
      return;

   const FSmartObjectSlotEQSItem& item = UEnvQueryItemType_SmartObject::GetValue(envQueryResult->GetItemRawMemory(0));
   
   FStateTreeEvent event;
   event.Tag = TAG_StateTreeEvent_DistractionEvent;
   event.Payload = FInstancedStruct::Make(FTATAITargetingEvent_DistractionEvent({item.SlotHandle}));
   
   _StateTreeAIComponent->SendStateTreeEvent(event);
}

void UTATStateTreeDistractionComponent::StartDelayTimer()
{
   const float calculatedDistractionDelay = _DistractionDelay + FMath::RandRange(-_DistractionDelayVariance, _DistractionDelayVariance);
   GetWorld()->GetTimerManager().SetTimer(_DistractionHandle,
                                          FTimerDelegate::CreateUObject(this, &ThisClass::OnDistractionPossible),
                                          calculatedDistractionDelay,
                                          false
                                          );
}

void UTATStateTreeDistractionComponent::SetDistractableState(bool allowDistractions)
{
   _IsDistractionAllowed = allowDistractions;
   if(_IsDistractionAllowed)
   {
      if(_DistractionHandle.IsValid() == false)
      {
         StartDelayTimer();
      }
   }
   else
   {
      if(_DistractionHandle.IsValid())
      {
         StopDelayTimer();
      }
   }
}

void UTATStateTreeDistractionComponent::StopDelayTimer()
{
   GetWorld()->GetTimerManager().ClearTimer(_DistractionHandle);
}
