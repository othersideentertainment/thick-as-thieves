// (c) 2018-2019 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerState.h"
#include "GameFramework/Controller.h"
#include "GameFramework/GameState.h"
#include "OSECommon.generated.h"

class UAbilitySystemComponent;
struct FHitResult;

// useful enum for out-refs in function calls so blueprints along with ExpandEnumAsExecs to get a valid/invalid branching
UENUM(BlueprintType)
enum class EBranchValidity : uint8
{
   Valid,
   Invalid
};

/**
 * Common C++ and blueprint library to hold generally useful utility functions
 * Many things were moved here from CommonComponent
 */
UCLASS()
class OSECORE_API UOSECommon : public UBlueprintFunctionLibrary
{
   GENERATED_BODY()

public:

   // Shared C++ and Blueprint functions

   /** Creates a FName of form BaseString_154134 */
   UFUNCTION(BlueprintCallable, Category = "OSE")
   static FName GenerateUniqueID(const FString& BaseString);
   
   /** Faster raw version of above that skips one allocation */
   static FName GenerateUniqueID(const TCHAR* BaseString);
   
   /** Creates a FName of form BaseName_154134, faster than string version if existing name is available */
   UFUNCTION(BlueprintCallable, Category = "OSE")
   static FName GenerateUniqueIDFromName(FName BaseName);

   /** Returns character corresponding to actor, which can be a player state or controller */
   UFUNCTION(BlueprintPure, Category = "Character|OSE")
   static AOSECharacterBase* GetCharacterFromActor(AActor* Actor);

   /** Returns character corresponding to component, which can be attached to any related actor */
   UFUNCTION(BlueprintPure, Category = "Character|OSE")
   static AOSECharacterBase* GetCharacterFromComponent(UActorComponent* Component);

   /// Returns whether actor is a player-controlled pawn.
   UFUNCTION(BlueprintPure, Category = "Character|OSE")
   static bool IsAPlayer(AActor* actor);

   /// Returns the Physical Material associated with a Hit Result. This will attempt a variety of
   /// methods to discover the physmat, preferring more optimal discovery via cached data over
   /// performing new raytraces.
   UFUNCTION(BlueprintPure, Category = "Character|OSE")
   static UPhysicalMaterial* GetPhysicalMaterialFromHitResult(const FHitResult& hitResult);

   // Methods for pre-quantizing vectors so the the value will remain about the same after going over the network
   static FVector_NetQuantize PreQuantize(const FVector_NetQuantize& Vector);
   static FVector_NetQuantize10 PreQuantize(const FVector_NetQuantize10& Vector);
   static FVector_NetQuantize100 PreQuantize(const FVector_NetQuantize100& Vector);
   static void PreQuantizeHitResult(FHitResult& hit);
   
   // C++ Only templates

   /** Accessor to get pawn of specific class corresponding to actor, which can be a player state or controller */
   template <class TDst = APawn, class TSrc = AActor>
   static TDst* GetPawn(TSrc* Actor)
   {
      auto FoundPawn = Cast<TDst>(Actor);

      if (FoundPawn != nullptr)
      {
         return FoundPawn;
      }

      if (auto FoundController = Cast<AController>(Actor))
      {
         return FoundController->template GetPawn<TDst>();
      }

      if (auto FoundPlayerState = Cast<APlayerState>(Actor))
      {
         return FoundPlayerState->template GetPawn<TDst>();
      }

      return nullptr;
   }

   /** Accessor to get player state of specific class corresponding to actor, which can be a pawn or controller */
   template <class TDst = APlayerState, class TSrc = AActor>
   static TDst* GetPlayerState(TSrc* actor)
   {
      auto foundPlayerState = Cast<TDst>(actor);

      if (foundPlayerState != nullptr)
      {
         return foundPlayerState;
      }

      if (auto foundPawn = Cast<APawn>(actor))
      {
         return foundPawn->template GetPlayerState<TDst>();
      }

      if (auto foundController = Cast<AController>(actor))
      {
         return foundController->template GetPlayerState<TDst>();
      }

      return nullptr;
   }

   /** Accessor to get pawn of specific class from a component attached to a pawn, controller, or player state */
   template <class T = APawn>
   static T* GetPawnFromComponent(UActorComponent* Component)
   {
      if (Component != nullptr)
      {
         return GetPawn<T>(Component->GetOwner());
      }

      return nullptr;
   }

   /** Accessor to get game state from an actor */
   template<class T = AGameState>
   static inline T* GetGameState(AActor* Actor)
   {
      if (Actor && Actor->GetWorld())
      {
         return Actor->GetWorld()->GetGameState<T>();
      }

      return nullptr;
   }

   /** Accessor to get a specific component type by searching actor, controller, and player state */
   template<typename T>
   static T* GetComponent(const AActor* Actor)
   {
      if (Actor)
      {
         if (T* Component = Actor->FindComponentByClass<T>())
         {
            return Component;
         }

         if (const APawn* Pawn = Cast<APawn>(Actor))
         {
            if (const AController* Controller = Pawn->GetController())
            {
               if (T* Component = Controller->FindComponentByClass<T>())
               {
                  return Component;
               }
            }
            if (const APlayerState* PlayerState = Pawn->GetPlayerState())
            {
               if (T* Component = PlayerState->FindComponentByClass<T>())
               {
                  return Component;
               }
            }
         }
         else if (const AController* Controller = Cast<AController>(Actor))
         {
            return GetComponent<T>(Controller->GetPawn());
         }
         else if (const APlayerState* PlayerState = Cast<APlayerState>(Actor))
         {
            return GetComponent<T>(PlayerState->GetPawn());
         }
      }

      return nullptr;
   }

   /** Shuffles a TArray using a random stream. */
   template<typename ElementType, typename Allocator>
   static void ShuffleArray(TArray<ElementType, Allocator>& arr, const FRandomStream& randomStream)
   {
      for (int32 i = 0; i < arr.Num(); ++i)
      {
         int32 swapIdx = randomStream.RandRange(i, arr.Num() - 1);
         arr.Swap(i, swapIdx);
      }
   }

   /** Gets controller from actor that may be a pawn or controller */
   template<typename T = AController>
   static inline T* GetController(AActor* Actor)
   {
      if (T* Controller = Cast<T>(Actor))
      {
         return Controller;
      }
      else if (APawn* Pawn = Cast<APawn>(Actor))
      {
         return Pawn->GetController<T>();
      }
      return nullptr;
   }

   /** Gets controller from actor that may be a pawn or controller */
   template<typename T = AController>
   static inline const T* GetController(const AActor* Actor)
   {
      if (const T* Controller = Cast<T>(Actor))
      {
         return Controller;
      }
      else if (const APawn* Pawn = Cast<APawn>(Actor))
      {
         return Pawn->GetController<T>();
      }
      return nullptr;
   }

   /// Gets a player controller at a given index
   template<typename T = APlayerController>
   static inline T* GetPlayerControllerAtIndex(const UObject* worldContext, int index)
   {
      return Cast<T>(UGameplayStatics::GetPlayerController(worldContext, index));
   }

   /// Gets a local player controller at a given index
   template<typename T = APlayerController>
   static inline T* GetLocalPlayerControllerAtIndex(const UObject* worldContext, int index)
   {
      if (T* pc = GetPlayerControllerAtIndex<T>(worldContext, index))
      {
         if (pc->IsLocalController())
            return pc;
      }
      return nullptr;
   }

   /// Gets a local player controller given any world context object
   template<typename T = APlayerController>
   static inline T* GetLocalPlayerController(const UObject* worldContext)
   {
      // player controller 0 is going to be our local player controller in all games that are not split-screen.
      // for split screen games, we'll want to use the index version of this function above
      static const int kLocalPlayerNum = 0;
      return GetLocalPlayerControllerAtIndex<T>(worldContext, kLocalPlayerNum);
   }

   /// Looks for the root instigator, following the instigator chain
   /// Stops when an actor:
   ///  - has no instigator
   ///  - has itself as its instigator
   /// Other cycles, i.e. longer than single-actors, will assert and return null
   /// returns nullptr if no none is found
   static AActor* FindUltimateInstigator(AActor* actor);

   /** Creates an integer bitmask out of a BitFlag enum value */
   template<typename EnumType>
   static inline uint32 EnumToFlags(EnumType a1)
   {
      return (1 << (uint8)a1);
   }

   /** Template version of above that combines multiple flags */
   template<typename EnumType, typename ...Ts>
   static inline uint32 EnumToFlags(EnumType a1, Ts... a2)
   {
      return (1 << (uint8)a1) | EnumToFlags(a2...);
   }

   /** Checks to see if any of the specified BitFlag enum values are set */
   template<typename ...Ts>
   static inline bool AreAnyFlagsSet(uint8 Flags, Ts... a1)
   {
      return (Flags & EnumToFlags(a1...)) > 0;
   }

   /** Checks to see if all of the specified BitFlag enum values are set */
   template<typename ...Ts>
   static inline bool AreAllFlagsSet(uint8 Flags, Ts... a1)
   {
      return (Flags & EnumToFlags(a1...)) == Flags;
   }

   // UEnum::GetValueAsString fully qualifies the value with the namespace, which is not always desirable.
   template<typename T>
   static inline FString UnqualifiedEnumToString(T enumValue)
   {
      return StaticEnum<T>()->GetNameStringByValue((int64)enumValue);
   }

private:
   /// Internal helper for GetPhysicalMaterialFromHitResult
   static UPhysicalMaterial* GetPhysicalMaterialFromComponent(const UPrimitiveComponent* primitiveComponent, int32 faceIndex);
};
