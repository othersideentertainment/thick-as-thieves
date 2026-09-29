// (c) 2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "TATGameInstanceSubsystem.generated.h"

//+jmb: DEPRECATED - WILL BE REMOVING SOON 

class UTATGameInstance;

#define IMPLEMENT_SINGLETON(Type)                                                 \
private:                                                                          \
    static Type* Singleton;                                                       \
public:                                                                           \
    static inline Type& Get() { return *Type::Singleton; }                        \
protected:                                                                        \
    virtual void _InitializeSingleton() override { Type::Singleton = this; }      \
    virtual void _DeinitializeSingleton() override { Type::Singleton = nullptr; }

#define DEFINE_SINGLETON(Type) Type* Type::Singleton = nullptr


UCLASS()
class TAT_API UTATGameInstanceSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& collection) override;

	virtual void Deinitialize() override;

	virtual void PostInitialize() {} // May be called in TATGameInstance::Init after Initializing all subsystems. Can access other GI Subsystems.

protected:
   virtual void _InitializeSingleton() {}
   virtual void _DeinitializeSingleton() {}

   TObjectPtr<UTATGameInstance> _GetTATGameInstance() const { return _TATGameInstance; }

protected:
   UPROPERTY(Transient)
   TObjectPtr<UTATGameInstance> _TATGameInstance = nullptr;
};

//-jmb: DEPRECATED - WILL BE REMOVING SOON
