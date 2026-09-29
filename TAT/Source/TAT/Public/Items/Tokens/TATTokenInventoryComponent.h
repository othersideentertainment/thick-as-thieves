// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue
#include "Components/ActorComponent.h"

#include "TATTokenInventoryComponent.generated.h"

struct FTATTokenEffect;
class UTATInventoryToken;

template<typename T = FTATTokenEffect>
struct TTATTokenEffectFindResult
{
   const T* Effect = nullptr;
   int32 Index = INDEX_NONE;

   bool IsValid() const
   {
      return Effect != nullptr && Index >= 0;
   }

   template<typename TOther>
   TTATTokenEffectFindResult<TOther> As() const
   {
      return { static_cast<const TOther*>(Effect), Index};
   }
};

// Component that tracks an inventory of tokens
//
// Tokens are simple non-droppable things that have side effects when the character holds them
UCLASS()
class TAT_API UTATTokenInventoryComponent : public UActorComponent
{
   GENERATED_BODY()

public:
   UTATTokenInventoryComponent();

   UFUNCTION(BlueprintCallable, Category="TAT", meta=(DisplayName="Get Token Inventory From Actor"))
   static UTATTokenInventoryComponent* FromActor(const AActor* actor);

   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Tokens")
   void AuthorityAddToken(const UTATInventoryToken* token);
   void _AuthorityOnTokenRemoved(const UTATInventoryToken* token);
   UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category="Tokens")
   void AuthorityRemoveToken(const UTATInventoryToken* token);
   void AuthorityRemoveTokenAt(int32 index);

   template<typename T, typename TFunc>
   TTATTokenEffectFindResult<T> FindTokenWithEffect(TFunc&& predicate)
   {
      return _FindTokenWithEffect(T::StaticStruct(), [&predicate] (const FTATTokenEffect& effect) { return predicate(static_cast<const T&>(effect));}).template As<T>();
   }

   DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTokensChanged);
   UPROPERTY(BlueprintAssignable, Transient)
   FOnTokensChanged OnTokensChanged;

   UFUNCTION(BlueprintPure, Category="Tokens")
   bool HasToken(const UTATInventoryToken* token) const { return _tokens.Contains(token); }
   UFUNCTION(BlueprintPure, Category="Tokens")
   int GetTokenCount() const { return _tokens.Num(); }
   UFUNCTION(BlueprintPure, Category="Tokens")
   const UTATInventoryToken* GetTokenAt(int index) const { return _tokens.IsValidIndex(index) ? _tokens[index] : nullptr; }

private:
   TTATTokenEffectFindResult<> _FindTokenWithEffect(const UScriptStruct* structType, TFunctionRef<bool (const FTATTokenEffect&)> predicate) const;
   
   UFUNCTION()
   void _OnRep_Tokens();
   
   // consider fast array if this is high traffic, but if this is high traffic
   // it also isn't the ideal data representation
   UPROPERTY(ReplicatedUsing=_OnRep_Tokens)
   TArray<TObjectPtr<const UTATInventoryToken>> _tokens;
};
