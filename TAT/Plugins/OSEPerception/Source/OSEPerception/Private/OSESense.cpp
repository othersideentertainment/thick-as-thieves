// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#include "OSESense.h"
#include "OSEPerceptionSystem.h"
#include "OSESenseConfig.h"
#include "VisualLogger/VisualLogger.h"
#include "OSESenseConfig_Hearing.h"
#include "OSESenseConfig_Prediction.h"
#include "OSESenseConfig_Sight.h"
#include "OSESenseConfig_Team.h"
#include "OSESenseConfig_Touch.h"
#include "OSEPerceptionComponent.h"
#include "OSESense_Prediction.h"
#include "OSESense_Touch.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(OSESense)

#if WITH_GAMEPLAY_DEBUGGER
#include "GameplayDebuggerTypes.h"
#include "GameplayDebuggerCategory.h"
#endif

const float UOSESense::SuspendNextUpdate = FLT_MAX;

UOSESense::UOSESense(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
	, TimeUntilNextUpdate(SuspendNextUpdate)
	, SenseID(FOSESenseID::InvalidID())
{

	bNeedsForgettingNotification = false;

	if (HasAnyFlags(RF_ClassDefaultObject) == false)
	{
		SenseID = ((const UOSESense*)GetClass()->GetDefaultObject())->GetSenseID();
	}
}

UWorld* UOSESense::GetWorld() const
{
	return PerceptionSystemInstance ? PerceptionSystemInstance->GetWorld() : nullptr;
}

void UOSESense::HardcodeSenseID(TSubclassOf<UOSESense> SenseClass, FOSESenseID HardcodedID)
{
	UOSESense* MutableCDO = GetMutableDefault<UOSESense>(SenseClass);
	check(MutableCDO);
	MutableCDO->SenseID = HardcodedID;
}

void UOSESense::PostInitProperties() 
{
	Super::PostInitProperties();

	if (HasAnyFlags(RF_ClassDefaultObject) == false) 
	{
		PerceptionSystemInstance = Cast<UOSEPerceptionSystem>(GetOuter());
	}
}

OSEPerception::FListenerMap* UOSESense::GetListeners() 
{
	check(PerceptionSystemInstance);
	return &(PerceptionSystemInstance->GetListenersMap());
}

void UOSESense::OnNewPawn(APawn& NewPawn)
{
	if (WantsNewPawnNotification())
	{
		UE_VLOG(GetPerceptionSystem(), LogOSEPerception, Warning
			, TEXT("%s declars it needs New Pawn notification but does not override OnNewPawn"), *GetName());		
	}		
}

void UOSESense::SetSenseID(FOSESenseID Index)
{
	check(Index != FOSESenseID::InvalidID());
	SenseID = Index;
}

void UOSESense::ForceSenseID(FOSESenseID InSenseID)
{
	check(GetClass()->HasAnyClassFlags(CLASS_CompiledFromBlueprint) == true);
	ensure(GetClass()->HasAnyClassFlags(CLASS_Abstract) == false);
	
	SenseID = InSenseID;
}

FOSESenseID UOSESense::UpdateSenseID()
{
	check(HasAnyFlags(RF_ClassDefaultObject) == true && GetClass()->HasAnyClassFlags(CLASS_Abstract | CLASS_CompiledFromBlueprint) == false);

	if (SenseID.IsValid() == false)
	{
		SenseID = FOSESenseID(GetFName());
	}

	return SenseID;
}

void UOSESense::RegisterWrappedEvent(UOSESenseEvent& PerceptionEvent)
{
	UE_VLOG(GetPerceptionSystem(), LogOSEPerception, Error, TEXT("%s did not override UOSESense::RegisterWrappedEvent!"), *GetName());
}

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
UOSESenseConfig::UOSESenseConfig(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer), DebugColor(FColor::White), bStartsEnabled(true)
{
}

TSubclassOf<UOSESense> UOSESenseConfig::GetSenseImplementation() const 
{ 
	return UOSESense::StaticClass(); 
}

FString UOSESenseConfig::GetSenseName() const
{
	if (CachedSenseName.Len() == 0 && GetSenseImplementation() != nullptr)
	{
		CachedSenseName = GetSenseImplementation()->GetName();
		CachedSenseName.RemoveFromEnd(TEXT("_C"));

		int32 SeparatorIdx = INDEX_NONE;
		const bool bHasSeparator = CachedSenseName.FindLastChar(TEXT('_'), SeparatorIdx);
		if (bHasSeparator)
		{
			CachedSenseName.MidInline(SeparatorIdx + 1, MAX_int32, EAllowShrinking::No);
		}
	}

	return CachedSenseName;
}

#if WITH_GAMEPLAY_DEBUGGER
static FString DescribeColorHelper(const FColor& Color)
{
	const int32 MaxColors = GColorList.GetColorsNum();
	for (int32 Idx = 0; Idx < MaxColors; Idx++)
	{
		if (Color == GColorList.GetFColorByIndex(Idx))
		{
			return GColorList.GetColorNameByIndex(Idx);
		}
	}

	return FString(TEXT("color"));
}

void UOSESenseConfig::DescribeSelfToGameplayDebugger(const UOSEPerceptionComponent* PerceptionComponent, FGameplayDebuggerCategory* DebuggerCategory) const
{
	if (DebuggerCategory)
	{
		DebuggerCategory->AddTextLine(
			FString::Printf(TEXT("%s: {%s}%s"), *GetSenseName(), *GetDebugColor().ToString(), *DescribeColorHelper(GetDebugColor()))
			);
	}
}
#endif // WITH_GAMEPLAY_DEBUGGER

//----------------------------------------------------------------------//
// 
//----------------------------------------------------------------------//
UOSESenseConfig_Sight::UOSESenseConfig_Sight(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DebugColor = FColor::Green;
}

TSubclassOf<UOSESense> UOSESenseConfig_Sight::GetSenseImplementation() const 
{ 
	return *Implementation; 
}	

#if WITH_EDITOR
void UOSESenseConfig_Sight::PostEditChangeChainProperty(FPropertyChangedChainEvent& PropertyChangedEvent)
{
	static const FName NAME_AutoSuccessRangeFromLastSeenLocation = GET_MEMBER_NAME_CHECKED(UOSESenseConfig_Sight, AutoSuccessRangeFromLastSeenLocation);

	Super::PostEditChangeProperty(PropertyChangedEvent);

	if (PropertyChangedEvent.Property)
	{
		const FName PropName = PropertyChangedEvent.Property->GetFName();
		if (PropName == NAME_AutoSuccessRangeFromLastSeenLocation)
		{
			if (AutoSuccessRangeFromLastSeenLocation < 0)
			{
				AutoSuccessRangeFromLastSeenLocation = FAISystem::InvalidRange;
			}
		}
	}
}
#endif // WITH_EDITOR

#if WITH_GAMEPLAY_DEBUGGER
void UOSESenseConfig_Sight::DescribeSelfToGameplayDebugger(const UOSEPerceptionComponent* PerceptionComponent, FGameplayDebuggerCategory* DebuggerCategory) const
{
	if (PerceptionComponent == nullptr || DebuggerCategory == nullptr)
	{
		return;
	}

	FColor SightRangeColor = FColor::Green;
	FColor LoseSightRangeColor = FColorList::NeonPink;

	// don't call Super implementation on purpose, replace color description line
	DebuggerCategory->AddTextLine(
		FString::Printf(TEXT("%s: {%s}%s {white}rangeIN:{%s} %.2f (%s) {white} rangeOUT:{%s} %.2f (%s)"), *GetSenseName(),
			*GetDebugColor().ToString(), *DescribeColorHelper(GetDebugColor()),
			*SightRangeColor.ToString(), SightRadius, *DescribeColorHelper(SightRangeColor),
			*LoseSightRangeColor.ToString(), LoseSightRadius, *DescribeColorHelper(LoseSightRangeColor))
		);

	const AActor* BodyActor = PerceptionComponent->GetBodyActor();
	if (BodyActor != nullptr)
	{
		FVector BodyLocation, BodyFacing;
		PerceptionComponent->GetLocationAndDirection(BodyLocation, BodyFacing);

		DebuggerCategory->AddShape(FGameplayDebuggerShape::MakeCylinder(BodyLocation, LoseSightRadius, 25.0f, LoseSightRangeColor));
		DebuggerCategory->AddShape(FGameplayDebuggerShape::MakeCylinder(BodyLocation, SightRadius, 25.0f, SightRangeColor));

		const float SightPieLength = FMath::Max(LoseSightRadius, SightRadius) + PointOfViewBackwardOffset;
		const FVector RootLocation = BodyLocation - (BodyFacing * PointOfViewBackwardOffset);
		const FVector LeftDirection = BodyFacing.RotateAngleAxis(PeripheralVisionAngleDegrees, FVector::UpVector);
		const FVector RightDirection = BodyFacing.RotateAngleAxis(-PeripheralVisionAngleDegrees, FVector::UpVector);
		DebuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegment(RootLocation + (BodyFacing * NearClippingRadius), RootLocation + (BodyFacing * SightPieLength), SightRangeColor));
		DebuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegment(RootLocation + (LeftDirection * NearClippingRadius), RootLocation + (LeftDirection * SightPieLength), SightRangeColor));
		DebuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegment(RootLocation + (RightDirection * NearClippingRadius), RootLocation + (RightDirection * SightPieLength), SightRangeColor));
		DebuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegment(RootLocation + (LeftDirection * NearClippingRadius), RootLocation + (BodyFacing * NearClippingRadius), SightRangeColor));
		DebuggerCategory->AddShape(FGameplayDebuggerShape::MakeSegment(RootLocation + (BodyFacing * NearClippingRadius), RootLocation + (RightDirection * NearClippingRadius), SightRangeColor));
	}
}
#endif // WITH_GAMEPLAY_DEBUGGER

//----------------------------------------------------------------------//
// UOSESenseConfig_Hearing
//----------------------------------------------------------------------//

UOSESenseConfig_Hearing::UOSESenseConfig_Hearing(const FObjectInitializer& ObjectInitializer) 
	: Super(ObjectInitializer), HearingRange(3000.f)
{
	DebugColor = FColor::Yellow;
}

TSubclassOf<UOSESense> UOSESenseConfig_Hearing::GetSenseImplementation() const 
{ 
	return *Implementation; 
}

#if WITH_GAMEPLAY_DEBUGGER
void UOSESenseConfig_Hearing::DescribeSelfToGameplayDebugger(const UOSEPerceptionComponent* PerceptionComponent, FGameplayDebuggerCategory* DebuggerCategory) const
{
	if (PerceptionComponent == nullptr || DebuggerCategory == nullptr)
	{
		return;
	}

	FColor HearingRangeColor = FColor::Yellow;
	FColor LoSHearingRangeColor = FColorList::Cyan;

	// don't call Super implementation on purpose, replace color description line
	DebuggerCategory->AddTextLine(
		FString::Printf(TEXT("%s: {%s}%s {white}range:{%s}%s {white} rangeLoS:{%s}%s"), *GetSenseName(),
			*GetDebugColor().ToString(), *DescribeColorHelper(GetDebugColor()),
			*HearingRangeColor.ToString(), *DescribeColorHelper(HearingRangeColor),
			*LoSHearingRangeColor.ToString(), *DescribeColorHelper(LoSHearingRangeColor))
		);

	const AActor* BodyActor = PerceptionComponent->GetBodyActor();
	if (BodyActor != nullptr)
	{
		FVector OwnerLocation = BodyActor->GetActorLocation();
		
		DebuggerCategory->AddShape(FGameplayDebuggerShape::MakeCylinder(OwnerLocation, HearingRange, 25.0f, HearingRangeColor));
		if (bUseLoSHearing)
		{
			DebuggerCategory->AddShape(FGameplayDebuggerShape::MakeCylinder(OwnerLocation, LoSHearingRange, 25.0f, LoSHearingRangeColor));
		}
	}
}
#endif // WITH_GAMEPLAY_DEBUGGER

//----------------------------------------------------------------------//
// UOSESenseConfig_Prediction
//----------------------------------------------------------------------//
UOSESenseConfig_Prediction::UOSESenseConfig_Prediction(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DebugColor = FColorList::Grey;
}

TSubclassOf<UOSESense> UOSESenseConfig_Prediction::GetSenseImplementation() const 
{ 
	return UOSESense_Prediction::StaticClass(); 
}


UOSESenseConfig_Team::UOSESenseConfig_Team(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DebugColor = FColor::Blue;
}

UOSESenseConfig_Touch::UOSESenseConfig_Touch(const FObjectInitializer& ObjectInitializer) : Super(ObjectInitializer)
{
	DebugColor = FColor::Cyan;
}

TSubclassOf<UOSESense> UOSESenseConfig_Touch::GetSenseImplementation() const
{
	return UOSESense_Touch::StaticClass();
}

