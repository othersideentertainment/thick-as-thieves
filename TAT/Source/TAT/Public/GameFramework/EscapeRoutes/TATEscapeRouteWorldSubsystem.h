// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Interactables/TATEscapePoint.h"
#include "Subsystems/WorldSubsystem.h"
#include "TATEscapeRouteWorldSubsystem.generated.h"

class APlayerState;

UCLASS(BlueprintType)
class TAT_API UTATEscapeRouteWorldSubsystem : public UWorldSubsystem
{
   GENERATED_BODY()
   DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEscapeRouteChange, ATATEscapePoint*, escapePoint);

   DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEscapeRouteUsed, ATATEscapePoint*, escapePoint, APlayerState*, player);
public:
   
   UFUNCTION(BlueprintCallable)
   const TSet<ATATEscapePoint*>& GetAllCurrentlyValidEscapePoints() const;
   
   void RegisterEscapePoint(ATATEscapePoint* escapePoint);
   void UnregisterEscapePoint(ATATEscapePoint* escapePoint);

   void AuthorityBroadcastEscapeRouteUsed(ATATEscapePoint* escapePoint, APlayerState* player);

   /// These two events are for when escape routes are loaded/unloaded in the map
   FOnEscapeRouteChange OnEscapeRouteAdded;
   FOnEscapeRouteChange OnEscapeRouteRemoved;

   /// When an escape route is now opened/usable
   UPROPERTY(BlueprintAssignable)
   FOnEscapeRouteChange OnEscapeRouteOpened;

   /// When an escape route has closed and is no longer usable
   UPROPERTY(BlueprintAssignable)
   FOnEscapeRouteChange OnEscapeRouteClosed;

   UPROPERTY(BlueprintAssignable)
   FOnEscapeRouteUsed AuthorityOnEscapeRouteUsed;
   
protected:
   bool _hasFirstEscapeRouteOpened { false };
   
   UPROPERTY(Transient)
   TSet<ATATEscapePoint*> _registeredEscapePoints {};
};
