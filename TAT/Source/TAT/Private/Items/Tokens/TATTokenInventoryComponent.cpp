// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Items/Tokens/TATTokenInventoryComponent.h"

// tat
#include "Items/Tokens/TATInventoryToken.h"
#include "Items/Tokens/TATTokenEffect.h"

// ue
#include "Net/UnrealNetwork.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATTokenInventoryComponent)


UTATTokenInventoryComponent::UTATTokenInventoryComponent()
{
   PrimaryComponentTick.bCanEverTick = false;
   SetIsReplicatedByDefault(true);
}

UTATTokenInventoryComponent* UTATTokenInventoryComponent::FromActor(const AActor* actor)
{
   if(actor == nullptr)
   {
      return nullptr;
   }

   // TODO: maybe have interface, so it can delegate?
   return actor->GetComponentByClass<UTATTokenInventoryComponent>();
}

void UTATTokenInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
   Super::GetLifetimeReplicatedProps(OutLifetimeProps);

   DOREPLIFETIME_CONDITION(UTATTokenInventoryComponent, _tokens, COND_OwnerOnly);
}

void UTATTokenInventoryComponent::AuthorityAddToken(const UTATInventoryToken* token)
{
   check(GetOwner()->HasAuthority());
   if(!ensure(token))
   {
      return;
   }

   _tokens.Add(token);
   if(const FTATTokenEffect* effect = token->Effect.GetPtr<FTATTokenEffect>())
   {
      effect->OnAdded(GetOwner());
   }
   OnTokensChanged.Broadcast();
}

void UTATTokenInventoryComponent::_AuthorityOnTokenRemoved(const UTATInventoryToken* token)
{
   if(const FTATTokenEffect* effect = token->Effect.GetPtr<FTATTokenEffect>())
   {
      effect->OnRemoved(GetOwner());
   }
   OnTokensChanged.Broadcast();
}

void UTATTokenInventoryComponent::AuthorityRemoveToken(const UTATInventoryToken* token)
{
   check(GetOwner()->HasAuthority());
   if(!ensure(token))
   {
      return;
   }
   
   if(_tokens.RemoveSingle(token) > 0)
   {
      _AuthorityOnTokenRemoved(token);
   }
}

void UTATTokenInventoryComponent::AuthorityRemoveTokenAt(int32 index)
{
   check(GetOwner()->HasAuthority());
   if(!_tokens.IsValidIndex(index))
   {
      return;
   }

   const UTATInventoryToken* token = _tokens[index];
   if(!ensure(token))
   {
      return;
   }

   _tokens.RemoveAt(index);
   _AuthorityOnTokenRemoved(token);
}

TTATTokenEffectFindResult<> UTATTokenInventoryComponent::_FindTokenWithEffect(const UScriptStruct* structType, TFunctionRef<bool(const FTATTokenEffect&)> predicate) const
{
   for(int i = 0; i < _tokens.Num(); ++i)
   {
      const UTATInventoryToken* token = _tokens[i];
      if(!ensure(token))
      {
         continue;
      }

      // can allow subtyping when there is a usecase
      if(token->Effect.GetScriptStruct() != structType)
      {
         continue;
      }

      const FTATTokenEffect& effect = token->Effect.Get<FTATTokenEffect>();
      if(predicate(effect))
      {
         return {&effect, i};
      }
   }

   return {};
}

void UTATTokenInventoryComponent::_OnRep_Tokens()
{
   OnTokensChanged.Broadcast();
}

