// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSEPerceptionStimuliSourceComponent.h"
#include "OSEPerceptionSystem.h"
#include "VisualLogger/VisualLogger.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSEPerceptionStimuliSourceComponent)


UOSEPerceptionStimuliSourceComponent::UOSEPerceptionStimuliSourceComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, bAutoRegisterAsSource(false)
{
	bSuccessfullyRegistered = false;
}

void UOSEPerceptionStimuliSourceComponent::OnRegister()
{
	Super::OnRegister();

#if WITH_EDITOR
	// when in the editor world we don't remove the null entries
	// since those can get changed to something else by the user
	if (!GIsEditor || GIsPlayInEditorWorld)
#endif // WITH_EDITOR
	{
		RegisterAsSourceForSenses.RemoveAllSwap([](const TSubclassOf<UOSESense>& SenseClass) {
			return SenseClass == nullptr;
		});
	}

	if (bAutoRegisterAsSource)
	{
		RegisterWithPerceptionSystem();
	}
}

void UOSEPerceptionStimuliSourceComponent::RegisterWithPerceptionSystem()
{
	if (bSuccessfullyRegistered)
	{
		return;
	}
	if (RegisterAsSourceForSenses.Num() == 0)
	{
		bSuccessfullyRegistered = true;
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	UWorld* World = OwnerActor->GetWorld();
	if (World)
	{
		UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(World);
		if (PerceptionSystem)
		{
			for (auto& SenseClass : RegisterAsSourceForSenses)
			{
				if(SenseClass)
				{
					PerceptionSystem->RegisterSourceForSenseClass(SenseClass, *OwnerActor);
					bSuccessfullyRegistered = true;
				}
				// we just ignore the empty entries
			}
		}
	}
}

void UOSEPerceptionStimuliSourceComponent::RegisterForSense(TSubclassOf<UOSESense> SenseClass)
{
	if (!SenseClass)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	UWorld* World = OwnerActor->GetWorld();
	if (World)
	{
		UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(World);
		if (PerceptionSystem)
		{
			UE_CVLOG(bSuccessfullyRegistered == false && RegisterAsSourceForSenses.Num(), OwnerActor, LogOSEPerception, Warning
				, TEXT("Registering as stimuli source for sense %s while the UOSEPerceptionStimuliSourceComponent has not registered with the OSEPerceptionSystem just yet. This will result in posing as a source for only this one sense.")
				, *SenseClass->GetName());

			PerceptionSystem->RegisterSourceForSenseClass(SenseClass, *OwnerActor);
			RegisterAsSourceForSenses.AddUnique(SenseClass);
			bSuccessfullyRegistered = true;
		}
	}
}

void UOSEPerceptionStimuliSourceComponent::UnregisterFromPerceptionSystem()
{
	if (bSuccessfullyRegistered == false)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	UWorld* World = OwnerActor->GetWorld();
	if (World)
	{
		UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(World);
		if (PerceptionSystem)
		{
			for (auto& SenseClass : RegisterAsSourceForSenses)
			{
				PerceptionSystem->UnregisterSource(*OwnerActor, SenseClass);
			}
		}
	}

	bSuccessfullyRegistered = false;
}

void UOSEPerceptionStimuliSourceComponent::UnregisterFromSense(TSubclassOf<UOSESense> SenseClass)
{
	if (!SenseClass)
	{
		return;
	}

	AActor* OwnerActor = GetOwner();
	if (OwnerActor == nullptr)
	{
		return;
	}

	UWorld* World = OwnerActor->GetWorld();
	if (World)
	{
		UOSEPerceptionSystem* PerceptionSystem = UOSEPerceptionSystem::GetCurrent(World);
		if (PerceptionSystem)
		{
			PerceptionSystem->UnregisterSource(*OwnerActor, SenseClass);
			RegisterAsSourceForSenses.RemoveSingleSwap(SenseClass, EAllowShrinking::No);
			bSuccessfullyRegistered = RegisterAsSourceForSenses.Num() > 0;
		}
	}
}

#if WITH_EDITOR
void UOSEPerceptionStimuliSourceComponent::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	static const FName NAME_RegisterAsSourceForSenses = GET_MEMBER_NAME_CHECKED(UOSEPerceptionStimuliSourceComponent, RegisterAsSourceForSenses);

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property != nullptr)
	{
		if (PropertyChangedEvent.Property->GetFName() == NAME_RegisterAsSourceForSenses)
		{
			if (PropertyChangedEvent.ChangeType == EPropertyChangeType::Duplicate)
			{
				const int32 ChangeAtIndex = PropertyChangedEvent.GetArrayIndex(NAME_RegisterAsSourceForSenses.ToString());
				if (ensure(ChangeAtIndex != INDEX_NONE))
				{
					// clear duplicate
					RegisterAsSourceForSenses[ChangeAtIndex] = nullptr;
				}
			}
			else if (PropertyChangedEvent.ChangeType == EPropertyChangeType::ValueSet)
			{
				TArray<TSubclassOf<UOSESense>> TmpCopy = RegisterAsSourceForSenses;
				RegisterAsSourceForSenses.Empty(RegisterAsSourceForSenses.Num());
				for (TSubclassOf<UOSESense> Sense : TmpCopy)
				{
					if (Sense)
					{
						RegisterAsSourceForSenses.AddUnique(Sense);
					}
				}
			}
		}
	}
}
#endif // WITH_EDITOR

