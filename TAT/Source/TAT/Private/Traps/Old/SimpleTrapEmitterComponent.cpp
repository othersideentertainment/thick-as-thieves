// (c) 2018-2020 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Traps/Old/SimpleTrapEmitterComponent.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(SimpleTrapEmitterComponent)

// Sets default values for this component's properties
USimpleTrapEmitterComponent_Old::USimpleTrapEmitterComponent_Old()
{
   PrimaryComponentTick.bCanEverTick = false;
   SetIsReplicatedByDefault(true);

   // ...
}

void USimpleTrapEmitterComponent_Old::OnTriggered_Implementation()
{
   AuthorityOnTriggered();
   ClientOnTriggered();
}

#if WITH_EDITOR
void USimpleTrapEmitterComponent_Old::CheckForErrors()
{
   // Only call into the mesh-component's implementation if it actually has a mesh
   if (GetStaticMesh())
   {
      Super::CheckForErrors();
   }
}
#endif

void USimpleTrapEmitterComponent_Old::AuthorityOnTriggered_Implementation()
{
}

void USimpleTrapEmitterComponent_Old::ClientOnTriggered_Implementation()
{
   K2_LocalOnTriggered();
}


