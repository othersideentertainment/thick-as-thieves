// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"

#include "TATCompassRegistrySubsystem.generated.h"

class ATATGenericIndicator;

// A world subsystem that keeps track of active compass indicators
//
// That way they can register themselves without worrying about lifetime issues
UCLASS()
class TAT_API UTATCompassRegistrySubsystem : public UWorldSubsystem
{
   GENERATED_BODY()

public:
   virtual bool ShouldCreateSubsystem(UObject* outer) const override;
	virtual bool DoesSupportWorldType(const EWorldType::Type worldType) const override;

   void RegisterIndicator(ATATGenericIndicator* indicator);
   void UnregisterIndicator(ATATGenericIndicator* indicator);

   const TSet<TWeakObjectPtr<ATATGenericIndicator>>& GetIndicators() const { return _registeredIndicators; }
   

   DECLARE_MULTICAST_DELEGATE_OneParam(FOnIndicatorAddRemove, ATATGenericIndicator*);
   FOnIndicatorAddRemove OnIndicatorAdded;
   FOnIndicatorAddRemove OnIndicatorRemoved;

private:
   // Could probably been fine as an array, but won't be iterated much
   TSet<TWeakObjectPtr<ATATGenericIndicator>> _registeredIndicators;
};
