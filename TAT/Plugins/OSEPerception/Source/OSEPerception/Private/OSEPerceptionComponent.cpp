// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSEPerceptionComponent.h"
#include "GameFramework/Controller.h"
#include "AIController.h"
#include "OSESenseConfig.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPerceptionComponent)

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerTypes.h"
#include "GameplayDebuggerCategory.h"
#endif



DECLARE_CYCLE_STAT(TEXT("Perception Component ProcessStimuli"), STAT_AI_PercepComp_ProcessStimuli, STATGROUP_AI);

DECLARE_CYCLE_STAT(TEXT("Requesting UOSEPerceptionComponent::RemoveDeadData call from within a const function"),
	STAT_FSimpleDelegateGraphTask_RequestingRemovalOfDeadPerceptionData,
	STATGROUP_TaskGraphTasks);

//----------------------------------------------------------------------//
// FOSEActorPerceptionInfo
//----------------------------------------------------------------------//
void FOSEActorPerceptionInfo::Merge(const FOSEActorPerceptionInfo& Other)
{
	for (uint32 Index = 0; Index < FOSESenseID::GetSize(); ++Index)
	{
		if (LastSensedStimuli[Index].GetAge() > Other.LastSensedStimuli[Index].GetAge())
		{
			LastSensedStimuli[Index] = Other.LastSensedStimuli[Index];
		}
	}
}

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
FOSEPerceptionBlueprintInfo::FOSEPerceptionBlueprintInfo(const FOSEActorPerceptionInfo& Info)
{
	Target = Info.Target.Get();
	LastSensedStimuli = Info.LastSensedStimuli;
	bIsHostile = Info.bIsHostile;
}

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
FOSEPerceptionUpdateInfo::FOSEPerceptionUpdateInfo(const int32 InTargetId, const TWeakObjectPtr<AActor>& InTarget, const FOSEStimulus& InStimulus)
	: TargetId(InTargetId)
	, Target(InTarget)
	, Stimulus(InStimulus)
{
}

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
const int32 UOSEPerceptionComponent::InitialStimuliToProcessArraySize = 10;

UOSEPerceptionComponent::UOSEPerceptionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, PerceptionListenerId(FOSEPerceptionListenerID::InvalidID())
	, bCleanedUp(false)
{
	bForgetStaleActors = GET_AI_CONFIG_VAR(bForgetStaleActors);
}

void UOSEPerceptionComponent::RequestStimuliListenerUpdate()
{
	UOSEPerceptionSystem* OSEPerceptionSys = UOSEPerceptionSystem::GetCurrent(GetWorld());
	if (OSEPerceptionSys != NULL)
	{
		OSEPerceptionSys->UpdateListener(*this);
	}
}

namespace
{
	struct FConfigOfSenseID
	{
		FConfigOfSenseID(const FOSESenseID& InSenseID)
			: SenseID(InSenseID)
		{}

		bool operator()(const UOSESenseConfig* SenseConfig) const
		{
			return SenseConfig && SenseConfig->GetSenseID() == SenseID;
		}

		const FOSESenseID SenseID;
	};
}

const UOSESenseConfig* UOSEPerceptionComponent::GetSenseConfig(const FOSESenseID& SenseID) const
{
	int32 ConfigIndex = SensesConfig.IndexOfByPredicate(FConfigOfSenseID(SenseID));
	return ConfigIndex != INDEX_NONE ? SensesConfig[ConfigIndex] : nullptr;
}

UOSESenseConfig* UOSEPerceptionComponent::GetSenseConfig(const FOSESenseID& SenseID)
{
	int32 ConfigIndex = SensesConfig.IndexOfByPredicate(FConfigOfSenseID(SenseID));
	return ConfigIndex != INDEX_NONE ? SensesConfig[ConfigIndex] : nullptr;
}

void UOSEPerceptionComponent::PostInitProperties() 
{
	Super::PostInitProperties();

	if (DominantSense)
	{
		DominantSenseID = UOSESense::GetSenseID(DominantSense);
	}
}

void UOSEPerceptionComponent::ConfigureSense(UOSESenseConfig& Config)
{
	// first check if we're reconfiguring a sense
	bool bIsNewConfig = true;
	for (TObjectPtr<UOSESenseConfig>& SenseConfig : SensesConfig)
	{
		if (SenseConfig != nullptr && SenseConfig->GetClass() == Config.GetClass())
		{
			SenseConfig = &Config;
			bIsNewConfig = false;
			break;
		}
	}

	if (bIsNewConfig)
	{
		SensesConfig.Add(&Config);
	}

	if (IsRegistered())
	{
	    UOSEPerceptionSystem* OSEPerceptionSys = UOSEPerceptionSystem::GetCurrent(GetWorld());
	    if (OSEPerceptionSys != nullptr)
	    {
			if (bIsNewConfig)
			{
				RegisterSenseConfig(Config, *OSEPerceptionSys);
			}
			else
			{
				SetMaxStimulusAge(Config.GetSenseID(), Config.GetMaxAge());
			}
		    OSEPerceptionSys->OnListenerConfigUpdated(Config.GetSenseID(), *this);
	    }
	}
	// else the sense will be auto-configured during OnRegister
}

UOSEPerceptionComponent::TAISenseConfigConstIterator UOSEPerceptionComponent::GetSensesConfigIterator() const
{
	return ToRawPtrTArrayUnsafe(SensesConfig).CreateConstIterator();
}

void UOSEPerceptionComponent::SetMaxStimulusAge(FOSESenseID SenseID, float MaxAge)
{
	if (!ensureMsgf(SenseID.IsValid(), TEXT("Sense must exist to update max age")))
	{
		return;
	}

	if (MaxActiveAge.IsValidIndex(SenseID) == false)
	{
		MaxActiveAge.AddUninitialized(SenseID - MaxActiveAge.Num() + 1);
	}
	MaxActiveAge[SenseID] = MaxAge;

	// @todo process all data already gathered and see if any _still_active_ stimuli
	// got it's expiration prolonged, with SetExpirationAge
}

void UOSEPerceptionComponent::OnRegister()
{
	Super::OnRegister();

	bCleanedUp = false;

	AActor* Owner = GetOwner();
	if (Owner != nullptr)
	{
		Owner->OnEndPlay.AddUniqueDynamic(this, &UOSEPerceptionComponent::OnOwnerEndPlay);
		AIOwner = Cast<AAIController>(Owner);

		// Whilst it should be possible with some code changes, to make perception components work when being added to other AActors than AIControllers, it's not something Epic support.
		// OSE BEGIN - disable warning
		// This warning was added in 5.1, but there do not seem to be any changes to this component that would actually affect this behavior. TAT has been
		// using this component on non-controllers for security cameras, and does appear to continue to work. The only thing that woukd not is the explicit calls to the owner,
		// but these are only some of the callbacks.
		// UE_CVLOG_UELOG(!AIOwner && Owner->GetWorld() && (Owner->GetWorld()->WorldType != EWorldType::Editor), Owner, LogOSEPerception, Warning, TEXT("%s: Perception Component is being registered with %s, they are designed to work with AAIControllers!"), ANSI_TO_TCHAR(__FUNCTION__), *Owner->GetName());
		// OSE END
	}

	UOSEPerceptionSystem* OSEPerceptionSys = UOSEPerceptionSystem::GetCurrent(GetWorld());
	if (OSEPerceptionSys != nullptr)
	{
		PerceptionFilter.Clear();

		if (SensesConfig.Num() > 0)
		{
			// set up perception listener based on SensesConfig
			for (auto SenseConfig : SensesConfig)
			{
				if (SenseConfig)
				{
					RegisterSenseConfig(*SenseConfig, *OSEPerceptionSys);
				}
			}

			OSEPerceptionSys->UpdateListener(*this);
		}
	}

	//// this should not be needed but aparently AAIController::PostRegisterAllComponents
	//// gets called component's OnRegister
	//AIOwner = Cast<AAIController>(GetOwner());
	//ensure(AIOwner == nullptr || AIOwner->GetOSEPerceptionComponent() == nullptr || AIOwner->GetOSEPerceptionComponent() == this
	//	|| (AIOwner->GetWorld() && AIOwner->GetWorld()->WorldType != EWorldType::Editor));
	//if (AIOwner && AIOwner->GetOSEPerceptionComponent() == nullptr)
	//{
	//	AIOwner->SetPerceptionComponent(*this);
	//}
}

void UOSEPerceptionComponent::RegisterSenseConfig(UOSESenseConfig& SenseConfig, UOSEPerceptionSystem& OSEPerceptionSys)
{
	const TSubclassOf<UOSESense> SenseImplementation = SenseConfig.GetSenseImplementation();
	if (SenseImplementation)
	{
		// make sure it's registered with perception system
		const FOSESenseID SenseID = OSEPerceptionSys.RegisterSenseClass(SenseImplementation);
		check(SenseID.IsValid());

		if (SenseConfig.IsEnabled())
		{
			PerceptionFilter.AcceptChannel(SenseID);
		}

		SetMaxStimulusAge(SenseID, SenseConfig.GetMaxAge());
	}
}

void UOSEPerceptionComponent::OnUnregister()
{
	CleanUp();
	Super::OnUnregister();
}

void UOSEPerceptionComponent::OnOwnerEndPlay(AActor* Actor, EEndPlayReason::Type EndPlayReason)
{
	if (EndPlayReason != EEndPlayReason::EndPlayInEditor && EndPlayReason != EEndPlayReason::Quit)
	{
		CleanUp();
	}
}

void UOSEPerceptionComponent::CleanUp()
{
	if (bCleanedUp == false)
	{
		ForgetAll();

		UOSEPerceptionSystem* OSEPerceptionSys = UOSEPerceptionSystem::GetCurrent(GetWorld());
		if (OSEPerceptionSys != nullptr)
		{
			OSEPerceptionSys->UnregisterListener(*this);
			AActor* MutableBodyActor = GetMutableBodyActor();
			if (MutableBodyActor)
			{
				OSEPerceptionSys->UnregisterSource(*MutableBodyActor);
			}
		}

		if (HasAnyFlags(RF_BeginDestroyed) == false)
		{
			AActor* Owner = GetOwner();
			if (Owner != nullptr)
			{
				Owner->OnEndPlay.RemoveDynamic(this, &UOSEPerceptionComponent::OnOwnerEndPlay);
			}
		}

		bCleanedUp = true;
	}
}

void UOSEPerceptionComponent::BeginDestroy()
{
	if (!HasAnyFlags(RF_ClassDefaultObject))
	{
		CleanUp();
	}
	Super::BeginDestroy();
}

void UOSEPerceptionComponent::UpdatePerceptionAllowList(const FOSESenseID Channel, const bool bNewValue)
{
	// Return if we don't have a Sense Config as it doesn't make sense to update the perception allow list.
	// Also modifying this often requires the Sense Config further along the call stack.
	if (GetSenseConfig(Channel) == nullptr)
	{
		UE_VLOG_UELOG(GetOwner(), LogOSEPerception, Warning, TEXT("%s: %s: Channel has no Sense Config. Bailing out!!"), ANSI_TO_TCHAR(__FUNCTION__), *Channel.Name.ToString());
		return;
	}

	const bool bCurrentValue = PerceptionFilter.ShouldRespondToChannel(Channel);
	if (bNewValue != bCurrentValue)
	{
		bNewValue ? PerceptionFilter.AcceptChannel(Channel) : PerceptionFilter.FilterOutChannel(Channel);
		RequestStimuliListenerUpdate();
	}
}

bool UOSEPerceptionComponent::GetFilteredActors(TFunctionRef<bool(const FOSEActorPerceptionInfo&)> Predicate, TArray<AActor*>& OutActors) const
{
	bool bDeadDataFound = false;

	OutActors.Reserve(PerceptualData.Num());
	for (FActorPerceptionContainer::TConstIterator DataIt = GetPerceptualDataConstIterator(); DataIt; ++DataIt)
	{
		const FOSEActorPerceptionInfo& ActorPerceptionInfo = DataIt->Value;
		if (Predicate(ActorPerceptionInfo))
		{
			if (AActor* Actor = ActorPerceptionInfo.Target.Get())
			{
				OutActors.Add(Actor);
			}
			else
			{
				bDeadDataFound = true;
			}
		}
	}
	return bDeadDataFound;
}

void UOSEPerceptionComponent::GetHostileActors(TArray<AActor*>& OutActors) const
{
	const bool bDeadDataFound = GetFilteredActors([](const FOSEActorPerceptionInfo& ActorPerceptionInfo) {
			return (ActorPerceptionInfo.bIsHostile && ActorPerceptionInfo.HasAnyKnownStimulus());
		}, OutActors);

	if (bDeadDataFound)
	{
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateUObject(const_cast<UOSEPerceptionComponent*>(this), &UOSEPerceptionComponent::RemoveDeadData),
			GET_STATID(STAT_FSimpleDelegateGraphTask_RequestingRemovalOfDeadPerceptionData), NULL, ENamedThreads::GameThread);
	}
}

void UOSEPerceptionComponent::GetHostileActorsBySense(TSubclassOf<UOSESense> SenseToFilterBy, TArray<AActor*>& OutActors) const
{
	const FOSESenseID SenseIdFilter = UOSESense::GetSenseID(SenseToFilterBy);

	if (SenseIdFilter == FOSESenseID::InvalidID())
	{
		UE_VLOG(GetOwner(), LogOSEPerception, Warning, TEXT("UOSEPerceptionComponent::GetHostileActorsBySense called with an invalid or yet unregistered sense. Bailing out."));
		return;
	}

	const bool bDeadDataFound = GetFilteredActors([SenseIdFilter](const FOSEActorPerceptionInfo& ActorPerceptionInfo) {
		return (ActorPerceptionInfo.bIsHostile && ActorPerceptionInfo.HasKnownStimulusOfSense(SenseIdFilter));
		}, OutActors);

	if (bDeadDataFound)
	{
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateUObject(const_cast<UOSEPerceptionComponent*>(this), &UOSEPerceptionComponent::RemoveDeadData),
			GET_STATID(STAT_FSimpleDelegateGraphTask_RequestingRemovalOfDeadPerceptionData), NULL, ENamedThreads::GameThread);
	}
}

const FOSEActorPerceptionInfo* UOSEPerceptionComponent::GetFreshestTrace(const FOSESenseID Sense) const
{
	// @note will stop on first age 0 stimulus
	float BestAge = FOSEStimulus::NeverHappenedAge;
	const FOSEActorPerceptionInfo* Result = NULL;

	bool bDeadDataFound = false;
	
	for (FActorPerceptionContainer::TConstIterator DataIt = GetPerceptualDataConstIterator(); DataIt; ++DataIt)
	{
		const FOSEActorPerceptionInfo* Info = &DataIt->Value;
		const float Age = Info->LastSensedStimuli[Sense].GetAge();
		if (Age < BestAge)
		{
			if (Info->Target.IsValid())
			{
				BestAge = Age;
				Result = Info;
				if (BestAge == 0.f)
				{
					// won't find any younger then this
					break;
				}
			}
			else
			{
				bDeadDataFound = true;
			}
		}
	}

	if (bDeadDataFound)
	{
		FSimpleDelegateGraphTask::CreateAndDispatchWhenReady(
			FSimpleDelegateGraphTask::FDelegate::CreateUObject(const_cast<UOSEPerceptionComponent*>(this), &UOSEPerceptionComponent::RemoveDeadData),
			GET_STATID(STAT_FSimpleDelegateGraphTask_RequestingRemovalOfDeadPerceptionData), NULL, ENamedThreads::GameThread);
	}

	return Result;
}

void UOSEPerceptionComponent::SetDominantSense(TSubclassOf<UOSESense> InDominantSense)
{
	if (DominantSense != InDominantSense)
	{
		DominantSense = InDominantSense;
		DominantSenseID = UOSESense::GetSenseID(InDominantSense);
		// update all perceptual info with this info
		for (FActorPerceptionContainer::TIterator DataIt = GetPerceptualDataIterator(); DataIt; ++DataIt)
		{
			DataIt->Value.DominantSense = DominantSenseID;
		}
	}
}

FGenericTeamId UOSEPerceptionComponent::GetTeamIdentifier() const
{
	return FGenericTeamId::GetTeamIdentifier(GetOwner());
}

FVector UOSEPerceptionComponent::GetActorLocation(const AActor& Actor) const 
{ 
	// not that Actor == NULL is valid
	const FOSEActorPerceptionInfo* ActorInfo = GetActorInfo(Actor);
	return ActorInfo ? ActorInfo->GetLastStimulusLocation() : FAISystem::InvalidLocation;
}

void UOSEPerceptionComponent::GetLocationAndDirection(FVector& Location, FVector& Direction) const
{
	const AActor* OwnerActor = Cast<AActor>(GetOuter());
	if (OwnerActor != nullptr)
	{
		FRotator ViewRotation(ForceInitToZero);
		OwnerActor->GetActorEyesViewPoint(Location, ViewRotation);
		Direction = ViewRotation.Vector();
	}
}

const AActor* UOSEPerceptionComponent::GetBodyActor() const
{
	AController* OwnerController = Cast<AController>(GetOuter());
	if (OwnerController != NULL)
	{
		return OwnerController->GetPawn();
	}

	return Cast<AActor>(GetOuter());
}

AActor* UOSEPerceptionComponent::GetMutableBodyActor()
{
	return const_cast<AActor*>(GetBodyActor());
}

void UOSEPerceptionComponent::RegisterStimulus(AActor* Source, const FOSEStimulus& Stimulus)
{
	FStimulusToProcess& StimulusToProcess = StimuliToProcess.Add_GetRef(FStimulusToProcess(Source, Stimulus));
	StimulusToProcess.Stimulus.SetExpirationAge(MaxActiveAge[int32(Stimulus.Type)]);
}

void UOSEPerceptionComponent::ProcessStimuli()
{
	SCOPE_CYCLE_COUNTER(STAT_AI_PercepComp_ProcessStimuli);
	
	if(StimuliToProcess.Num() == 0)
	{
		UE_VLOG(GetOwner(), LogOSEPerception, Warning, TEXT("UOSEPerceptionComponent::ProcessStimuli called without any Stimuli to process"));
		return;
	}

	TArray<FStimulusToProcess> ProcessingStimuli = MoveTemp(StimuliToProcess);
	TArray<AActor*> UpdatedActors;
	UpdatedActors.Reserve(ProcessingStimuli.Num());
	TArray<AActor*> ActorsToForget;
	ActorsToForget.Reserve(ProcessingStimuli.Num());
	TArray<TObjectKey<AActor>, TInlineAllocator<8>> DataToRemove;

	for (FStimulusToProcess& SourcedStimulus : ProcessingStimuli)
	{
		const TObjectKey<AActor>& SourceKey = SourcedStimulus.Source;

		FOSEActorPerceptionInfo* PerceptualInfo = PerceptualData.Find(SourceKey);
		AActor* SourceActor = nullptr;

		if (PerceptualInfo == NULL)
		{
			if (SourcedStimulus.Stimulus.WasSuccessfullySensed() == false)
			{
				// this means it's a failed perception of an actor our owner is not aware of
				// at all so there's no point in creating perceptual data for a failed stimulus
				continue;
			}
			else
			{
				SourceActor = CastChecked<AActor>(SourceKey.ResolveObjectPtr(), ECastCheckedType::NullAllowed);

				// no existing perceptual data and source no longer valid: nothing to do with this stimulus
				if (SourceActor == nullptr)
				{
					continue;
				}
				
				// create an entry
				PerceptualInfo = &PerceptualData.Add(SourceKey, FOSEActorPerceptionInfo(SourceActor));
				// tell it what's our dominant sense
				PerceptualInfo->DominantSense = DominantSenseID;

				PerceptualInfo->bIsHostile = (FGenericTeamId::GetAttitude(GetOwner(), SourceActor) == ETeamAttitude::Hostile);
			}
		}

		if (PerceptualInfo->LastSensedStimuli.Num() <= SourcedStimulus.Stimulus.Type)
		{
			const int32 NumberToAdd = SourcedStimulus.Stimulus.Type - PerceptualInfo->LastSensedStimuli.Num() + 1;
			PerceptualInfo->LastSensedStimuli.AddDefaulted(NumberToAdd);
		}

		check(SourcedStimulus.Stimulus.Type.IsValid());

		FOSEStimulus& StimulusStore = PerceptualInfo->LastSensedStimuli[SourcedStimulus.Stimulus.Type];
		const bool bActorInfoUpdated = SourcedStimulus.Stimulus.WantsToNotifyOnlyOnPerceptionChange() == false 
			|| SourcedStimulus.Stimulus.WasSuccessfullySensed() != StimulusStore.WasSuccessfullySensed()
         || (StimulusStore.IsExpired() && SourcedStimulus.Stimulus.WantsToNotifyOnExpired());

		if (SourcedStimulus.Stimulus.WasSuccessfullySensed())
		{
			RefreshStimulus(StimulusStore, SourcedStimulus.Stimulus);
		}
		else if (StimulusStore.IsExpired() == false)
		{	
			if (bActorInfoUpdated)
			{
				// @note there some more valid info in SourcedStimulus->Stimulus regarding test that failed
				// may be useful in future
				StimulusStore.MarkNoLongerSensed();
				StimulusStore.SetStimulusAge(0);
			}
		}
		else
		{
			HandleExpiredStimulus(StimulusStore);

			if (bForgetStaleActors && !PerceptualInfo->HasAnyCurrentStimulus())
			{
				if (AActor* ActorToForget = PerceptualInfo->Target.Get())
				{
					ActorsToForget.Add(ActorToForget);
				}
			}
		}

		// if the new stimulus is "valid" or it's info that "no longer sensed" and it used to be sensed successfully
		if (bActorInfoUpdated)
		{
			// Source Actor is only resolved from SourceKey when required but might already have been resolved for new entry
			SourceActor = (SourceActor == nullptr) ? CastChecked<AActor>(SourceKey.ResolveObjectPtr(), ECastCheckedType::NullAllowed) : SourceActor;
			if (SourceActor == nullptr)
			{
				DataToRemove.Add(SourceKey);
			}
			else
			{
				UpdatedActors.AddUnique(SourceActor);
            if(OnActorPerceptionUpdated.IsBound())
            {
               OnActorPerceptionUpdated.Broadcast(SourceActor, StimulusStore);
            }
			}
		}
	}

	if (UpdatedActors.Num() > 0)
	{
		if (AIOwner != NULL)
		{
			AIOwner->ActorsPerceptionUpdated(UpdatedActors);
		}
	}

	// forget actors that are no longer perceived
	for (AActor* ActorToForget : ActorsToForget)
	{
		ForgetActor(ActorToForget);
	}

	// remove perceptual info related to stale actors
	for (const TObjectKey<AActor>& SourceKey : DataToRemove)
	{
		PerceptualData.Remove(SourceKey);
	}
}

void UOSEPerceptionComponent::RefreshStimulus(FOSEStimulus& StimulusStore, const FOSEStimulus& NewStimulus)
{
	// if new stimulus is younger or stronger
	// note that stimulus Age depends on PerceptionSystem::PerceptionAgingRate. It's possible that 
	// both already stored and the new stimulus have Age of 0, but stored stimulus' acctual age is in [0, PerceptionSystem::PerceptionAgingRate)
	if (NewStimulus.GetAge() <= StimulusStore.GetAge() || StimulusStore.Strength < NewStimulus.Strength)
	{
		StimulusStore = NewStimulus;
		// update stimulus 
	}
}

void UOSEPerceptionComponent::HandleExpiredStimulus(FOSEStimulus& StimulusStore)
{
	ensure(StimulusStore.IsExpired() == true);
}

bool UOSEPerceptionComponent::AgeStimuli(const float ConstPerceptionAgingRate)
{
	bool bExpiredStimuli = false;

	for (FActorPerceptionContainer::TIterator It(PerceptualData); It; ++It)
	{
		FOSEActorPerceptionInfo& ActorPerceptionInfo = It->Value;

		for (FOSEStimulus& Stimulus : ActorPerceptionInfo.LastSensedStimuli)
		{
			// Age the stimulus. If it is active but has just expired, mark it as such
			if (Stimulus.AgeStimulus(ConstPerceptionAgingRate) == false
				&& (Stimulus.IsActive() || Stimulus.WantsToNotifyOnlyOnPerceptionChange())
				&& Stimulus.IsExpired() == false)
			{
				AActor* TargetActor = ActorPerceptionInfo.Target.Get();
				if (TargetActor)
				{
					Stimulus.MarkExpired();
					RegisterStimulus(TargetActor, Stimulus);
					bExpiredStimuli = true;
				}
			}
		}
	}

	return bExpiredStimuli;
}

void UOSEPerceptionComponent::ForgetActor(AActor* ActorToForget)
{
	if (PerceptualData.Num() > 0)
	{
		UOSEPerceptionSystem* OSEPerceptionSys = UOSEPerceptionSystem::GetCurrent(GetWorld());
		if (OSEPerceptionSys != nullptr && ActorToForget != nullptr)
		{
			OSEPerceptionSys->OnListenerForgetsActor(*this, *ActorToForget);
		}

		const int32 NumRemoved = PerceptualData.Remove(ActorToForget);
	}
}

void UOSEPerceptionComponent::ForgetAll()
{
	if (PerceptualData.Num() > 0)
	{
		UOSEPerceptionSystem* OSEPerceptionSys = UOSEPerceptionSystem::GetCurrent(GetWorld());
		if (OSEPerceptionSys != nullptr)
		{
			OSEPerceptionSys->OnListenerForgetsAll(*this);
		}

		PerceptualData.Reset();
	}
}

float UOSEPerceptionComponent::GetYoungestStimulusAge(const AActor& Source) const
{
	const FOSEActorPerceptionInfo* Info = GetActorInfo(Source);
	if (Info == NULL)
	{
		return FOSEStimulus::NeverHappenedAge;
	}

	float SmallestAge = FOSEStimulus::NeverHappenedAge;
	for (int32 SenseID = 0; SenseID < Info->LastSensedStimuli.Num(); ++SenseID)
	{
		if (Info->LastSensedStimuli[SenseID].WasSuccessfullySensed())
		{
			float SenseAge = Info->LastSensedStimuli[SenseID].GetAge();
			if (SenseAge < SmallestAge)
			{
				SmallestAge = SenseAge;
			}
		}
	}

	return SmallestAge;
}

bool UOSEPerceptionComponent::HasAnyActiveStimulus(const AActor& Source) const
{
	const FOSEActorPerceptionInfo* Info = GetActorInfo(Source);
	if (Info == NULL)
	{
		return false;
	}

	return Info->HasAnyKnownStimulus();
}

bool UOSEPerceptionComponent::HasAnyCurrentStimulus(const AActor& Source) const
{
	const FOSEActorPerceptionInfo* Info = GetActorInfo(Source);
	if (Info == NULL)
	{
		return false;
	}

	return Info->HasAnyCurrentStimulus();
}

bool UOSEPerceptionComponent::HasActiveStimulus(const AActor& Source, FOSESenseID Sense) const
{
	const FOSEActorPerceptionInfo* Info = GetActorInfo(Source);
	return (Info 
		&& Info->LastSensedStimuli.IsValidIndex(Sense) 
		&& Info->LastSensedStimuli[Sense].WasSuccessfullySensed()
		&& Info->LastSensedStimuli[Sense].GetAge() < FOSEStimulus::NeverHappenedAge
		&& (Info->LastSensedStimuli[Sense].GetAge() <= MaxActiveAge[Sense] || MaxActiveAge[Sense] == 0.f));
}

void UOSEPerceptionComponent::RemoveDeadData()
{
	for (FActorPerceptionContainer::TIterator It(PerceptualData); It; ++It)
	{
		if (It->Value.Target.IsValid() == false)
		{
			It.RemoveCurrent();
		}
	}
}

//----------------------------------------------------------------------//
// blueprint interface
//----------------------------------------------------------------------//
void UOSEPerceptionComponent::GetPerceivedHostileActors(TArray<AActor*>& OutActors) const
{
	GetHostileActors(OutActors);
}

void UOSEPerceptionComponent::GetPerceivedHostileActorsBySense(const TSubclassOf<UOSESense> SenseToUse, TArray<AActor*>& OutActors) const
{
	GetHostileActorsBySense(SenseToUse, OutActors);
}

void UOSEPerceptionComponent::GetCurrentlyPerceivedActors(TSubclassOf<UOSESense> SenseToUse, TArray<AActor*>& OutActors) const
{
	const FOSESenseID SenseID = UOSESense::GetSenseID(SenseToUse);

	OutActors.Reserve(PerceptualData.Num());
	for (FActorPerceptionContainer::TConstIterator DataIt = GetPerceptualDataConstIterator(); DataIt; ++DataIt)
	{
		const bool bCurrentlyPerceived = (SenseToUse == nullptr) ? DataIt->Value.HasAnyCurrentStimulus() : DataIt->Value.IsSenseActive(SenseID);
		if (bCurrentlyPerceived)
		{
			if (AActor* Actor = DataIt->Value.Target.Get())
			{
				OutActors.Add(Actor);
			}
		}
	}
}

void UOSEPerceptionComponent::GetKnownPerceivedActors(TSubclassOf<UOSESense> SenseToUse, TArray<AActor*>& OutActors) const
{
	const FOSESenseID SenseID = UOSESense::GetSenseID(SenseToUse);

	OutActors.Reserve(PerceptualData.Num());
	for (FActorPerceptionContainer::TConstIterator DataIt = GetPerceptualDataConstIterator(); DataIt; ++DataIt)
	{
		const bool bWasEverPerceived = (SenseToUse == nullptr) ? DataIt->Value.HasAnyKnownStimulus() : DataIt->Value.HasKnownStimulusOfSense(SenseID);
		if (bWasEverPerceived)
		{
			if (DataIt->Value.Target.IsValid())
			{
				OutActors.Add(DataIt->Value.Target.Get());
			}
		}
	}
}

bool UOSEPerceptionComponent::GetActorsPerception(AActor* Actor, FOSEPerceptionBlueprintInfo& Info)
{
	bool bInfoFound = false;
	if (Actor != nullptr && Actor->IsPendingKillPending() == false)
	{
		const FOSEActorPerceptionInfo* PerceivedInfo = GetActorInfo(*Actor);
		if (PerceivedInfo)
		{
			Info = FOSEPerceptionBlueprintInfo(*PerceivedInfo);
			bInfoFound = true;
		}
	}

	return bInfoFound;
}

bool UOSEPerceptionComponent::GetLastSensedActorLocation(AActor * actor, TSubclassOf<UOSESense> senseToUse, FVector & lastSensedLocation) const
{
   if (!IsValid(actor))
   {
      return false;
   }

   if (const FOSEActorPerceptionInfo* perceivedInfo = GetActorInfo(*actor))
   {
      const FOSESenseID senseID = UOSESense::GetSenseID(senseToUse);
      lastSensedLocation = senseToUse ? perceivedInfo->GetStimulusLocation(senseID) : perceivedInfo->GetLastStimulusLocation();
      return true;
   }
   return false;
}


void UOSEPerceptionComponent::SetSenseEnabled(TSubclassOf<UOSESense> SenseClass, const bool bEnable)
{
	const FOSESenseID SenseID = UOSESense::GetSenseID(SenseClass);
	if (SenseID.IsValid())
	{
		UpdatePerceptionAllowList(SenseID, bEnable);
	}
}

//----------------------------------------------------------------------//
// debug
//----------------------------------------------------------------------//
#if WITH_GAMEPLAY_DEBUGGER
void UOSEPerceptionComponent::DescribeSelfToGameplayDebugger(FGameplayDebuggerCategory* DebuggerCategory) const
{
	if (DebuggerCategory == nullptr)
	{
		return;
	}
 
	for (UOSEPerceptionComponent::FActorPerceptionContainer::TConstIterator It(GetPerceptualDataConstIterator()); It; ++It)
	{
      int PrintCount = 0;
		const FOSEActorPerceptionInfo& ActorPerceptionInfo = It->Value;
		const AActor* Target = ActorPerceptionInfo.Target.Get();
		if (Target != nullptr)
		{
			const FVector TargetLocation = Target->GetActorLocation();
			for (const FOSEStimulus& Stimulus : ActorPerceptionInfo.LastSensedStimuli)
			{
				const UOSESenseConfig* SenseConfig = GetSenseConfig(Stimulus.Type);
				if (Stimulus.IsValid() && (Stimulus.IsExpired() == false) && SenseConfig)
				{
					const FString Description = FString::Printf(TEXT("%s: %.2f age:%.2f"), *SenseConfig->GetSenseName(), Stimulus.Strength, Stimulus.GetAge());
					const FColor DebugColor = Stimulus.IsActive() ? SenseConfig->GetDebugColor() : FColor::Red;

					DebuggerCategory->AddShape(FGameplayDebuggerShape::MakePoint(Stimulus.StimulusLocation + FVector(0, 0, PrintCount++*30 ), 30.0f, DebugColor, Description));
					DebuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegment(Stimulus.ReceiverLocation, Stimulus.StimulusLocation, DebugColor));
					DebuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegment(TargetLocation, Stimulus.StimulusLocation, FColor::Black));
				}
			}
		}
	}

	for (UOSESenseConfig* SenseConfig : SensesConfig)
	{
		if (SenseConfig)
		{
			SenseConfig->DescribeSelfToGameplayDebugger(this, DebuggerCategory);
		}
	}
}
#endif // WITH_GAMEPLAY_DEBUGGER

#if ENABLE_VISUAL_LOG
void UOSEPerceptionComponent::DescribeSelfToVisLog(FVisualLogEntry* Snapshot) const
{

}
#endif // ENABLE_VISUAL_LOG
