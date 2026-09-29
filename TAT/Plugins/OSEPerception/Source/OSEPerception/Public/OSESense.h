// Copyright Epic Games, Inc. All Rights Reserved.
// (c) 2026 OtherSide Entertainment, Inc
// SPDX-License-Identifier: LicenseRef-Unreal-Engine-EULA AND MIT


#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Object.h"
#include "Templates/SubclassOf.h"
#include "OSEPerceptionTypes.h"
#include "OSESense.generated.h"

class APawn;
class UOSEPerceptionSystem;
class UOSESenseEvent;



UCLASS(ClassGroup = AI, abstract, config = Engine)
class OSEPERCEPTION_API UOSESense : public UObject
{
   DECLARE_DELEGATE_OneParam(FOnPerceptionListenerUpdateDelegate, const FOSEPerceptionListener&);

	GENERATED_UCLASS_BODY()

	static const float SuspendNextUpdate;

protected:
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OSE Perception", config)
	EOSESenseNotifyType NotifyType;

	/** whether this sense is interested in getting notified about new Pawns being spawned 
	 *	this can be used for example for automated sense sources registration */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OSE Perception", config)
	uint32 bWantsNewPawnNotification : 1;

	/** If true all newly spawned pawns will get auto registered as source for this sense. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "OSE Perception", config)
	uint32 bAutoRegisterAllPawnsAsSources : 1;

	/** this sense has some internal logic that requires it to be notified when 
	 *	a listener wants to forget an actor*/
	uint32 bNeedsForgettingNotification : 1;
	
private:
	UPROPERTY()
	TObjectPtr<UOSEPerceptionSystem> PerceptionSystemInstance;

	/** then this count reaches 0 sense will be updated */
	float TimeUntilNextUpdate;

	FOSESenseID SenseID;

protected:
	/**	If bound will be called when new FOSEPerceptionListener gets registers with OSEPerceptionSystem */
	FOnPerceptionListenerUpdateDelegate OnNewListenerDelegate;

	/**	If bound will be called when a FOSEPerceptionListener's in OSEPerceptionSystem change */
	FOnPerceptionListenerUpdateDelegate OnListenerUpdateDelegate;

	/**	If bound will be called when a FOSEPerceptionListener's in removed from OSEPerceptionSystem */
	FOnPerceptionListenerUpdateDelegate OnListenerRemovedDelegate;
				
public:

	virtual UWorld* GetWorld() const override;

	/** use with caution! Needs to be called before any senses get instantiated or listeners registered. DOES NOT update any perceptions system instances */
	static void HardcodeSenseID(TSubclassOf<UOSESense> SenseClass, FOSESenseID HardcodedID);

	static FOSESenseID GetSenseID(const TSubclassOf<UOSESense> SenseClass) { return SenseClass ? ((const UOSESense*)SenseClass->GetDefaultObject())->SenseID : FOSESenseID::InvalidID(); }
	template<typename TSense>
	static FOSESenseID GetSenseID() 
	{ 
		return GetDefault<TSense>()->GetSenseID();
	}
	FORCEINLINE FOSESenseID GetSenseID() const { return SenseID; }

	FORCEINLINE bool WantsUpdateOnlyOnPerceptionValueChange() const { return (NotifyType == EOSESenseNotifyType::OnPerceptionChange || NotifyType == EOSESenseNotifyType::OnPerceptionChangeAndExpire ); }
   FORCEINLINE bool WantsUpdateOnPerceptionExpire() const { return (NotifyType == EOSESenseNotifyType::OnPerceptionChangeAndExpire); }

	virtual void PostInitProperties() override;

	/** 
	 *	@return should this sense be ticked now
	 */
	bool ProgressTime(float DeltaSeconds)
	{
		TimeUntilNextUpdate -= DeltaSeconds;
		return TimeUntilNextUpdate <= 0.f;
	}

	void Tick()
	{
		if (TimeUntilNextUpdate <= 0.f)
		{
			TimeUntilNextUpdate = Update();
		}
	}

	//virtual void RegisterSources(TArray<AActor&> SourceActors) {}
	virtual void RegisterSource(AActor& SourceActors){}
	virtual void UnregisterSource(AActor& SourceActors){}

	virtual void RegisterWrappedEvent(UOSESenseEvent& PerceptionEvent);
	virtual FOSESenseID UpdateSenseID();

	bool NeedsNotificationOnForgetting() const { return bNeedsForgettingNotification; }
	virtual void OnListenerForgetsActor(const FOSEPerceptionListener& Listener, AActor& ActorToForget) {}
	virtual void OnListenerForgetsAll(const FOSEPerceptionListener& Listener) {}

	FORCEINLINE void OnNewListener(const FOSEPerceptionListener& NewListener) { OnNewListenerDelegate.ExecuteIfBound(NewListener); }
	FORCEINLINE void OnListenerUpdate(const FOSEPerceptionListener& UpdatedListener) { OnListenerUpdateDelegate.ExecuteIfBound(UpdatedListener); }
	FORCEINLINE void OnListenerRemoved(const FOSEPerceptionListener& RemovedListener) { OnListenerRemovedDelegate.ExecuteIfBound(RemovedListener); }
	virtual void OnListenerConfigUpdated(const FOSEPerceptionListener& UpdatedListener) { OnListenerUpdate(UpdatedListener); }

	bool WantsNewPawnNotification() const { return bWantsNewPawnNotification; }
	bool ShouldAutoRegisterAllPawnsAsSources() const { return bAutoRegisterAllPawnsAsSources; }

protected:
	friend UOSEPerceptionSystem;
	/** gets called when perception system gets notified about new spawned pawn. 
	 *	@Note: do not call super implementation. It's used to detect when subclasses don't override it */
	virtual void OnNewPawn(APawn& NewPawn);

	/** @return time until next update */
	virtual float Update() { return FLT_MAX; }

	/** will result in updating as soon as possible */
	FORCEINLINE void RequestImmediateUpdate() { TimeUntilNextUpdate = 0.f; }

	/** will result in updating in specified number of seconds */
	FORCEINLINE void RequestUpdateInSeconds(float UpdateInSeconds) { TimeUntilNextUpdate = UpdateInSeconds; }

	FORCEINLINE UOSEPerceptionSystem* GetPerceptionSystem() const { return PerceptionSystemInstance; }

	void SetSenseID(FOSESenseID Index);

	/** returning pointer rather then a reference to prevent users from
	 *	accidentally creating copies by creating non-reference local vars */
	OSEPerception::FListenerMap* GetListeners();

	/** To be called only for BP-generated classes */
	void ForceSenseID(FOSESenseID SenseID);
};
