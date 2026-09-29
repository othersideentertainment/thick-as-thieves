// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Thiefsign/TATThiefsignFunctionLibrary.h"

// tat
#include "Items/ToolHolderInterface.h"
#include "Player/TATCharacter.h"
#include "Thiefsign/TATThiefsignTags.h"

// ose
#include "Abilities/OSEAbilityFunctionLibrary.h"

// ue
#include "Components/SceneComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/AssetManager.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATThiefsignFunctionLibrary)
DEFINE_LOG_CATEGORY_STATIC(LogTATThiefsignFunctionLibrary, Log, All);

FGameplayEventData UTATThiefsignFunctionLibrary::BuildThiefsignAbilityEventData(FGameplayTag thiefsignIdentifier, FGameplayTag thiefsignAnimationIdentifier, AActor* instigator)
{
   FGameplayEventData eventData;
   eventData.TargetTags.AddTag(thiefsignIdentifier);
   eventData.TargetTags.AddTag(thiefsignAnimationIdentifier);
   eventData.Instigator = instigator;
   return eventData;
}

void UTATThiefsignFunctionLibrary::RetrieveThiefsignIdentifierFromEventData(const FGameplayEventData& eventData, FGameplayTag& thiefsignIdentifier)
{
   thiefsignIdentifier = UOSEAbilityFunctionLibrary::FindSingleChildTag(eventData.TargetTags, TAG_Thiefsign);
}

void UTATThiefsignFunctionLibrary::RetrieveThiefsignAnimationIdentifierFromEventData(const FGameplayEventData& eventData, FGameplayTag& thiefsignAnimationIdentifier)
{
   thiefsignAnimationIdentifier = UOSEAbilityFunctionLibrary::FindSingleChildTag(eventData.TargetTags, TAG_Animation_Character);
}

FGameplayCueParameters UTATThiefsignFunctionLibrary::BuildThiefsignCueParameters(FGameplayTag thiefsignIdentifier, AActor* instigator)
{
   FGameplayCueParameters parameters;
   parameters.AggregatedTargetTags.AddTag(thiefsignIdentifier);
   parameters.Instigator = instigator;
   return parameters;
}

void UTATThiefsignFunctionLibrary::RetrieveThiefsignIdentifierFromCueParameters(const FGameplayCueParameters& parameters, FGameplayTag& thiefsignIdentifier)
{
   thiefsignIdentifier = UOSEAbilityFunctionLibrary::FindSingleChildTag(parameters.AggregatedTargetTags, TAG_Thiefsign);
}

bool UTATThiefsignFunctionLibrary::FindThiefsignAttachmentFromCueParameters(const FGameplayCueParameters& parameters, const FTATThiefsignCharacterConfig& characterConfig, USceneComponent*& attachToComponent, FVector& worldLocation)
{
   const AActor* instigator = parameters.Instigator.Get();
   if (instigator == nullptr)
   {
      UE_LOG(LogTATThiefsignFunctionLibrary, Error, TEXT("ApplyThiefsignAttachmentFromCueParameters: Thiefsign cue params had null instigator!"));
      return false;
   }

   const ATATCharacter* tatCharacter = Cast<ATATCharacter>(instigator);
   if (tatCharacter == nullptr)
   {
      UE_LOG(LogTATThiefsignFunctionLibrary, Error, TEXT("ApplyThiefsignAttachmentFromCueParameters: Thiefsign cue params had non-TATCharacter class as instigator, instead using %s!")
         , *instigator->GetClass()->GetName());
      return false;
   }

   // Use IToolHolderInterface::GetToolRoot as means for finding the proper mesh to read socket location from
   const IToolHolderInterface* toolHolderInterface = Cast<IToolHolderInterface>(tatCharacter);
   check(toolHolderInterface);

   const bool isFirstPerson = tatCharacter->IsLocallyControlled();

   // The mesh component we want to pull the socket location out of
   const EMeshPerspective perspective = isFirstPerson ? EMeshPerspective::FirstPerson : EMeshPerspective::ThirdPerson;
   USkeletalMeshComponent* socketSkeletalMesh = Cast<USkeletalMeshComponent>(toolHolderInterface->GetToolRoot(perspective));

   // The mesh component we want to attach the VFX to
   // Always use third person mesh as attaching to it is more stable (i.e.less sporadic movement across the screen)
   USkeletalMeshComponent* attachSkeletalMesh = Cast<USkeletalMeshComponent>(toolHolderInterface->GetToolRoot(EMeshPerspective::ThirdPerson));

   if (socketSkeletalMesh != nullptr && attachSkeletalMesh != nullptr)
   {
      const FName socketName = isFirstPerson ? characterConfig.SymbolSpawnSocket1P : characterConfig.SymbolSpawnSocket3P;
      const float zOffset = isFirstPerson ? characterConfig.SymbolSpawnSocket1PZOffset : characterConfig.SymbolSpawnSocket3PZOffset;

      const FTransform worldTransform = socketSkeletalMesh->GetSocketTransform(socketName, ERelativeTransformSpace::RTS_World);

      attachToComponent = attachSkeletalMesh;
      worldLocation = worldTransform.GetTranslation() + FVector(0.0f, 0.0f, zOffset);
      return true;
   }
   else
   {
      UE_LOG(LogTATThiefsignFunctionLibrary, Error, TEXT("ApplyThiefsignAttachmentFromCueParameters: Failed to find mesh!"));
      return false;
   }
}

void UTATThiefsignFunctionLibrary::AppendThiefsignMaterialParams(UPARAM(Ref) FTATThiefsignMaterialParameters& baseParams, const FTATThiefsignMaterialParameters& appendParams)
{
   baseParams.AppendParams(appendParams);
}

void UTATThiefsignFunctionLibrary::CreateAndApplyThiefsignNiagaraSystemData(ETATThiefsignType type, const UObject* contextObject, const FGameplayTag& thiefsignIdentifier, bool isFirstPerson, USceneComponent* attachToComponent, FVector worldLocation)
{
   check(attachToComponent);

   UTATThiefsignSettings& thiefsignSettings = UTATThiefsignSettings::GetMutable();
   const bool loadSymbolsIfUnloaded = true;
   if (thiefsignSettings.AreSymbolsLoaded(loadSymbolsIfUnloaded))
   {
      const FTATThiefsignInfo* thiefsignInfo = thiefsignSettings.FindThiefsignInfo(thiefsignIdentifier);
      if (thiefsignInfo == nullptr)
      {
         UE_LOG(LogTATThiefsignFunctionLibrary, Error, TEXT("CreateAndApplyThiefsignNiagaraSystemData: Failed to find Thiefsign info for %s!")
            , *thiefsignIdentifier.ToString());
         return;
      }

      FTATThiefsignVFXConfig vfxConfig = (type == ETATThiefsignType::Decal) ? thiefsignSettings.DecalVFXConfig : thiefsignSettings.HandVFXConfig;
      vfxConfig.Merge((type == ETATThiefsignType::Decal) ? thiefsignInfo->DecalVFXConfigOverrides : thiefsignInfo->HandVFXConfigOverrides);

      TWeakObjectPtr<USceneComponent> weakAttachToComponent(attachToComponent);
      UAssetManager::GetStreamableManager().RequestAsyncLoad(vfxConfig.NiagaraSystemClass.ToSoftObjectPath(), [vfxConfig, isFirstPerson, weakAttachToComponent, worldLocation]()
      {
         if (USceneComponent* attachToComponent = weakAttachToComponent.Get())
         {
            AActor* actor = attachToComponent->GetOwner();
            check(actor);

            FFXSystemSpawnParameters spawnParams;
            spawnParams.SystemTemplate = vfxConfig.NiagaraSystemClass.Get();
            spawnParams.AttachToComponent = attachToComponent;
            spawnParams.Location = worldLocation;
            spawnParams.LocationType = EAttachLocation::KeepWorldPosition;
            spawnParams.bAutoActivate = true;
            spawnParams.bAutoDestroy = true;

            if (UNiagaraComponent* niagaraComponent = UNiagaraFunctionLibrary::SpawnSystemAttachedWithParams(spawnParams))
            {
               vfxConfig.NiagaraSystemParams.ApplyToNiagaraSystem(niagaraComponent, isFirstPerson, nullptr);
            }
            else
            {
               UE_LOG(LogTATThiefsignFunctionLibrary, Error, TEXT("CreateAndApplyThiefsignNiagaraSystemData: Failed to create niagara system!"));
            }
         }
         else
         {
            UE_LOG(LogTATThiefsignFunctionLibrary, Error, TEXT("CreateAndApplyThiefsignNiagaraSystemData: Failed to retrieve attachToComponent!"));
         }
      });
   }
   else
   {
      // Load the symbols table and then re-run this method
      FSimpleMulticastDelegate::FDelegate onLoadedCallback;
      onLoadedCallback.BindWeakLambda(contextObject, [type, contextObject, thiefsignIdentifier, isFirstPerson, attachToComponent, worldLocation]
      {
         CreateAndApplyThiefsignNiagaraSystemData(type, contextObject, thiefsignIdentifier, isFirstPerson, attachToComponent, worldLocation);
      });
      thiefsignSettings.CallOrRegisterSymbolsLoadedDelegate(onLoadedCallback);
   }
}
