// Copyright Epic Games, Inc. All Rights Reserved.
// (c) OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "UBTService_FocusPerception.h"

#include "OSEPerceptionComponent.h"

#include "GameFramework/Actor.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Object.h"
#include "BehaviorTree/Blackboard/BlackboardKeyType_Vector.h"
#include "AIController.h"



UUBTService_FocusPerception::UUBTService_FocusPerception(const FObjectInitializer& ObjectInitializer /*= FObjectInitializer::Get()*/)
{
	NodeName = "Set focus";

	bTickIntervals = false;
	INIT_SERVICE_NODE_NOTIFY_FLAGS();

	FocusPriority = EAIFocusPriority::Default;

	// accept only actors
	BlackboardKey.AddObjectFilter(this, GET_MEMBER_NAME_CHECKED(UUBTService_FocusPerception, BlackboardKey), AActor::StaticClass());
}

void UUBTService_FocusPerception::OnBecomeRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnBecomeRelevant(OwnerComp, NodeMemory);

	FBTFocusPerceptionMemory* MyMemory = CastInstanceNodeMemory<FBTFocusPerceptionMemory>(NodeMemory);
	check(MyMemory);
	MyMemory->Reset();

	AAIController* OwnerController = OwnerComp.GetAIOwner();
	UBlackboardComponent* MyBlackboard = OwnerComp.GetBlackboardComponent();
   UOSEPerceptionComponent* MyPerception = OwnerController->GetComponentByClass<UOSEPerceptionComponent>();
	
   SetFocus(OwnerController, MyBlackboard, MyPerception, MyMemory);


   if(MyBlackboard != nullptr)
   {
		const FBlackboard::FKey KeyID = BlackboardKey.GetSelectedKeyID();
		MyBlackboard->RegisterObserver(KeyID, this, FOnBlackboardChangeNotification::CreateUObject(this, &UUBTService_FocusPerception::OnBlackboardKeyValueChange));
	}
}

void UUBTService_FocusPerception::SetFocus(AAIController* OwnerController, const UBlackboardComponent* MyBlackboard, const UOSEPerceptionComponent* MyPerception, FBTFocusPerceptionMemory* MyMemory)
{
   if (OwnerController != nullptr && MyBlackboard != nullptr && MyPerception != nullptr)
   {
      if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass())
      {
         UObject* KeyValue = MyBlackboard->GetValue<UBlackboardKeyType_Object>(BlackboardKey.GetSelectedKeyID());
         AActor* TargetActor = Cast<AActor>(KeyValue);
         if (TargetActor)
         {
            FVector FocusLocation;
            if (MyPerception->GetLastSensedActorLocation(TargetActor, nullptr, FocusLocation))
            {
               OwnerController->SetFocalPoint(FocusLocation, FocusPriority);
               //OwnerController->SetFocus(TargetActor, FocusPriority);
               MyMemory->FocusActorSet = TargetActor;
               MyMemory->FocusLocationSet = FocusLocation;
               MyMemory->bActorSet = true;
            }
         }
      }
   }
}

void UUBTService_FocusPerception::OnCeaseRelevant(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
	Super::OnCeaseRelevant(OwnerComp, NodeMemory);

	FBTFocusPerceptionMemory* MyMemory = CastInstanceNodeMemory<FBTFocusPerceptionMemory>(NodeMemory);
	check(MyMemory);
	AAIController* OwnerController = OwnerComp.GetAIOwner();
	if (OwnerController != nullptr)
	{
		bool bClearFocus = false;
		if (MyMemory->bActorSet)
		{
			bClearFocus = (MyMemory->FocusLocationSet == OwnerController->GetFocalPointForPriority(FocusPriority));
		}

		if (bClearFocus)
		{
			OwnerController->ClearFocus(FocusPriority);
		}
	}

	UBlackboardComponent* BlackboardComp = OwnerComp.GetBlackboardComponent();
	if (BlackboardComp)
	{
		BlackboardComp->UnregisterObserversFrom(this);
	}
}

FString UUBTService_FocusPerception::GetStaticDescription() const
{
	FString KeyDesc("invalid");
	if (BlackboardKey.SelectedKeyType == UBlackboardKeyType_Object::StaticClass() ||
		BlackboardKey.SelectedKeyType == UBlackboardKeyType_Vector::StaticClass())
	{
		KeyDesc = BlackboardKey.SelectedKeyName.ToString();
	}

	return FString::Printf(TEXT("Set focus to %s"), *KeyDesc);
}

EBlackboardNotificationResult UUBTService_FocusPerception::OnBlackboardKeyValueChange(const UBlackboardComponent& Blackboard, FBlackboard::FKey ChangedKeyID)
{
	UBehaviorTreeComponent* OwnerComp = Cast<UBehaviorTreeComponent>(Blackboard.GetBrainComponent());
	AAIController* OwnerController = OwnerComp ? OwnerComp->GetAIOwner() : nullptr;
	if (OwnerController == nullptr)
	{
		return EBlackboardNotificationResult::ContinueObserving;
	}

	const int32 NodeInstanceIdx = OwnerComp->FindInstanceContainingNode(this);
	FBTFocusPerceptionMemory* MyMemory = CastInstanceNodeMemory<FBTFocusPerceptionMemory>(OwnerComp->GetNodeMemory(this, NodeInstanceIdx));
	MyMemory->Reset();
	OwnerController->ClearFocus(FocusPriority);
   UOSEPerceptionComponent* MyPerception = OwnerController->GetComponentByClass<UOSEPerceptionComponent>();

   SetFocus(OwnerController, &Blackboard, MyPerception, MyMemory);

	return EBlackboardNotificationResult::ContinueObserving;
}

#if WITH_EDITOR
FName UUBTService_FocusPerception::GetNodeIconName() const
{
return FName("BTEditor.Graph.BTNode.Service.DefaultFocus.Icon");
}
#endif //WITH_EDITOR
