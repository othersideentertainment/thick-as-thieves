// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "AI/Navigation/TATNavModifierChildrenComponent.h"

// tat
#include "AI/Navigation/TATNavModifierChildrenDataInterface.h"
#include "Developer/TATProjectSettings.h"

// ue
#include "AI/Navigation/NavigationRelevantData.h"
#include "AI/NavigationModifier.h"
#include "Misc/DataValidation.h"
#include "PhysicsEngine/BodySetup.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATNavModifierChildrenComponent)

DEFINE_LOG_CATEGORY_STATIC(LogTATNavModifierChildrenComponent, Log, All);

#if WITH_EDITOR
EDataValidationResult UTATNavModifierChildrenComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   if (RefreshInterval < 0.0f)
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("%s | UTATNavModifierChildrenComponent::RefreshInterval must be non-negative!")
         , *GetOwner()->GetName())));
   }

   return context.GetIssues().Num() > 0 ? EDataValidationResult::Invalid : result;
}
#endif // WITH_EDITOR

void UTATNavModifierChildrenComponent::BeginDestroy()
{
   if (UWorld* world = GetWorld())
   {
      world->GetTimerManager().ClearTimer(_lastRefreshTimeout);
   }

   Super::BeginDestroy();
}

void UTATNavModifierChildrenComponent::CalcAndCacheBounds() const
{
   const UTATProjectSettings& tatSettings = UTATProjectSettings::Get();

   const bool bUpdateDynamic = !TransformUpdateHandle.IsValid();

#if WITH_EDITOR
   bool bUpdateStatic = bUpdateDynamic;
   if (UWorld* world = GetWorld())
   {
      bUpdateStatic |= (world->WorldType == EWorldType::Editor || world->WorldType == EWorldType::EditorPreview);
   }
#else
   const bool bUpdateStatic = bUpdateDynamic;
#endif // WITH_EDITOR

   if (bUpdateDynamic)
   {
      TArray<UActorComponent*> dynamicModifierComponents = GetOwner()->GetComponentsByTag(UPrimitiveComponent::StaticClass(), tatSettings.NavModifiableDynamicComponentTag);
      for (UActorComponent* component : dynamicModifierComponents)
      {
         // TODO : work ITATNavModifierChildrenDataInterface offsets into dynamic modifiers?

         UPrimitiveComponent* primitive = CastChecked<UPrimitiveComponent>(component);

         primitive->TransformUpdated.AddUObject(
            const_cast<UTATNavModifierChildrenComponent*>(this), &UTATNavModifierChildrenComponent::OnDynamicComponentTransformUpdated);

         _hasDynamicModifier = true;
      }
   }

   if (bUpdateStatic)
   {
#if WITH_EDITOR
      // only in editor would this run more than once
      // so gate clearing it with the editor check
      _staticModifierBounds.Reset();
#endif // WITH_EDITOR
      FTransform navModifierOffsetTransform = FTransform::Identity;
      TArray<UActorComponent*> staticModifierComponents = GetOwner()->GetComponentsByTag(UPrimitiveComponent::StaticClass(), tatSettings.NavModifiableStaticComponentTag);
      for (UActorComponent* component : staticModifierComponents)
      {
         UPrimitiveComponent* primitive = CastChecked<UPrimitiveComponent>(component);

         if (primitive->IsRegistered())
         {
            if (primitive->IsCollisionEnabled())
            {
               navModifierOffsetTransform.SetIdentity();
               if (const ITATNavModifierChildrenDataInterface* dataInterface = Cast<ITATNavModifierChildrenDataInterface>(primitive))
               {
                  dataInterface->GetStaticNavModifierOffsetTransform(navModifierOffsetTransform);
               }
               AppendStaticComponentBounds(primitive, navModifierOffsetTransform, _staticModifierBounds);
            }
            else
            {
               UE_LOG(LogTATNavModifierChildrenComponent, Warning, TEXT("Attempted to add %s->%s to static modifiers with collision disabled! Disregarding...")
                  , *primitive->GetOwner()->GetName(), *primitive->GetName());
            }
         }
         else
         {
            UE_LOG(LogTATNavModifierChildrenComponent, Warning, TEXT("Attempted to add %s->%s to static modifiers before it was registered! Disregarding...")
               , *primitive->GetOwner()->GetName(), *primitive->GetName());
         }
      }

      for (FRotatedBox& bounds : _staticModifierBounds)
      {
         const FVector navModBoxOrigin = FTransform(bounds.Quat).InverseTransformPosition(bounds.Box.GetCenter());
         bounds.Box = FBox::BuildAABB(navModBoxOrigin, bounds.Box.GetExtent());
      }
   }

   Super::CalcAndCacheBounds();
}

void UTATNavModifierChildrenComponent::CalculateBounds() const
{
   // NOTE: not calling Super, as overriding that behavior
   // Much adapted from Super::CalculateBounds
   const AActor* myOwner = GetOwner();
   if (!myOwner)
   {
      return;
   }

   Bounds = FBox(ForceInit);
   ComponentBounds.Reset();

   const FName requiredTag = UTATProjectSettings::Get().NavModifiableDynamicComponentTag;
   for (UActorComponent* component : myOwner->GetComponents())
   {
      UPrimitiveComponent* primComp = Cast<UPrimitiveComponent>(component);
      if (primComp && primComp->IsRegistered() && primComp->ComponentHasTag(requiredTag) && primComp->IsCollisionEnabled())
      {
         UBodySetup* bodySetup = primComp->GetBodySetup();
         if (bodySetup)
         {
            Bounds += primComp->Bounds.GetBox();

            const FTransform& parentTM = primComp->GetComponentTransform();
            PopulateComponentBounds(parentTM, *bodySetup);
         }
      }
   }

   // CONSIDER: Do we actually want this?
   if (ComponentBounds.Num() == 0)
   {
      Bounds = FBox::BuildAABB(myOwner->GetActorLocation(), FailsafeExtent);
      ComponentBounds.Add(FRotatedBox(Bounds, myOwner->GetActorQuat()));
   }

   for (int32 idx = 0; idx < ComponentBounds.Num(); idx++)
   {
      const FVector boxOrigin = ComponentBounds[idx].Box.GetCenter();
      const FVector boxExtent = ComponentBounds[idx].Box.GetExtent();

      const FVector navModBoxOrigin = FTransform(ComponentBounds[idx].Quat).InverseTransformPosition(boxOrigin);
      ComponentBounds[idx].Box = FBox::BuildAABB(navModBoxOrigin, boxExtent);
   }
}

void UTATNavModifierChildrenComponent::GetNavigationData(FNavigationRelevantData& data) const
{
   // if we only have static modifiers, we want to prevent the base
   // class from provide its failsafe extent box
   if (_hasDynamicModifier)
   {
      Super::GetNavigationData(data);
   }

   for (const FRotatedBox& bounds : _staticModifierBounds)
   {
      data.Modifiers.Add(FAreaNavModifier(bounds.Box, FTransform(bounds.Quat), AreaClass).SetIncludeAgentHeight(bIncludeAgentHeight));
   }
}

void UTATNavModifierChildrenComponent::AppendStaticComponentBounds(UPrimitiveComponent* primitive, const FTransform& offset, TArray<FRotatedBox>& bounds) const
{
   // largely copied from UNavModifierComponent::PopulateComponentBounds
   // outside of the application of the 'offset' param

   UBodySetup* bodySetup = primitive->GetBodySetup();
   if (bodySetup)
   {
      FTransform parentTM = primitive->GetComponentTransform();
      const FVector scale3D = parentTM.GetScale3D();
      parentTM.RemoveScaling();
      Bounds += primitive->Bounds.GetBox();

      for (int32 sphereIdx = 0; sphereIdx < bodySetup->AggGeom.SphereElems.Num(); sphereIdx++)
      {
         const FKSphereElem& elemInfo = bodySetup->AggGeom.SphereElems[sphereIdx];
         FTransform elemTM = elemInfo.GetTransform();
         elemTM.ScaleTranslation(scale3D);
         elemTM *= offset.Inverse();
         elemTM *= parentTM;

         const FBox sphereBounds = FBox::BuildAABB(elemTM.GetLocation(), elemInfo.Radius * scale3D);
         bounds.Add(FRotatedBox(sphereBounds, elemTM.GetRotation()));
      }

      for (int32 boxIdx = 0; boxIdx < bodySetup->AggGeom.BoxElems.Num(); boxIdx++)
      {
         const FKBoxElem& elemInfo = bodySetup->AggGeom.BoxElems[boxIdx];
         FTransform elemTM = elemInfo.GetTransform();
         elemTM.ScaleTranslation(scale3D);
         elemTM *= offset;
         elemTM *= parentTM;
         
         const FBox boxBounds = FBox::BuildAABB(elemTM.GetLocation(), FVector(elemInfo.X, elemInfo.Y, elemInfo.Z) * scale3D * 0.5f);
         bounds.Add(FRotatedBox(boxBounds, elemTM.GetRotation()));
      }

      for (int32 sphylIdx = 0; sphylIdx < bodySetup->AggGeom.SphylElems.Num(); sphylIdx++)
      {
         const FKSphylElem& elemInfo = bodySetup->AggGeom.SphylElems[sphylIdx];
         FTransform elemTM = elemInfo.GetTransform();
         elemTM.ScaleTranslation(scale3D);
         elemTM *= offset.Inverse();
         elemTM *= parentTM;

         const FBox sphylBounds = FBox::BuildAABB(elemTM.GetLocation(), FVector(elemInfo.Radius, elemInfo.Radius, elemInfo.Length) * scale3D);
         bounds.Add(FRotatedBox(sphylBounds, elemTM.GetRotation()));
      }

      for (int32 convexIdx = 0; convexIdx < bodySetup->AggGeom.ConvexElems.Num(); convexIdx++)
      {
         const FKConvexElem& elemInfo = bodySetup->AggGeom.ConvexElems[convexIdx];
         FTransform elemTM = elemInfo.GetTransform();
         elemTM *= offset.Inverse();

         const FBox convexBounds = FBox::BuildAABB(parentTM.TransformPosition(elemInfo.ElemBox.GetCenter() * scale3D), elemInfo.ElemBox.GetExtent() * scale3D);
         bounds.Add(FRotatedBox(convexBounds, elemTM.GetRotation() * parentTM.GetRotation()));
      }
   }
}

void UTATNavModifierChildrenComponent::OnDynamicComponentTransformUpdated(USceneComponent* component, EUpdateTransformFlags updateTransformFlags, ETeleportType teleport)
{
   const float currentTime = GetWorld()->GetTimeSeconds();
   if (currentTime >= _lastRefreshTime + RefreshInterval)
   {
      _lastRefreshTime = currentTime;

      // call the parent method to actually update the navmesh
      // the method has names pertaining to the root component, but should work fine for any component
      OnTransformUpdated(component, updateTransformFlags, teleport);

      // clear the timeout (which "queues" subsequent calls) when we refresh
      GetWorld()->GetTimerManager().ClearTimer(_lastRefreshTimeout);
   }
   else
   {
      // queue any extra calls we receive in the middle of refresh intervals to ensure we don't
      // miss any "final" updates to navmesh-affecting transform updates
      _lastRefreshComponent = component;
      GetWorld()->GetTimerManager().SetTimer(_lastRefreshTimeout
         , this, &UTATNavModifierChildrenComponent::_OnLastRefreshTimeoutElapsed, RefreshInterval, false);
   }
}

void UTATNavModifierChildrenComponent::_OnLastRefreshTimeoutElapsed()
{
   if (USceneComponent* lastRefreshComponent = _lastRefreshComponent.Get())
   {
      // this method should only trigger once the RefreshInterval has been passed
      // so there shouldn't be worry of an infinite loop between the two methods

      // we don't really care about these parameters, so don't worry about caching them
      // we can just use dummy values
      EUpdateTransformFlags dummyUpdateTransformFlags = EUpdateTransformFlags::None;
      ETeleportType dummyTeleportType = ETeleportType::None;
      OnDynamicComponentTransformUpdated(lastRefreshComponent, dummyUpdateTransformFlags, dummyTeleportType);
   }
}
