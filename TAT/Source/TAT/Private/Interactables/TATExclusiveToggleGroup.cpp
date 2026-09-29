// (c) 2018-2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT


#include "Interactables/TATExclusiveToggleGroup.h"

// ose
#include "Interactables/OSEInteractableToggle.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATExclusiveToggleGroup)


ATATExclusiveToggleGroup::ATATExclusiveToggleGroup()
{
   PrimaryActorTick.bCanEverTick = false;

   USceneComponent* sceneComp = CreateDefaultSubobject<USceneComponent>(TEXT("RootComponent"));
   RootComponent = sceneComp;
#if WITH_EDITORONLY_DATA
   RootComponent->bVisualizeComponent = true;
#endif
   sceneComp->Mobility = EComponentMobility::Static;

#if WITH_EDITORONLY_DATA
   _visComponent = CreateEditorOnlyDefaultSubobject<UTATExclusiveToggleGroupVisComponent>(TEXT("EditorVis"));
#endif
}

void ATATExclusiveToggleGroup::RegisterToggle(AOSESyncedToggle* toggle)
{
   if(!ensure(toggle))
   {
      return;
   }

   check(!_toggles.Contains(toggle));
   _toggles.Add(toggle);

   toggle->OnToggleStateChanged.AddUniqueDynamic(this, &ThisClass::_OnToggleStateChanged);

   if(toggle->IsOn())
   {
      _UpdateActiveToggle();
   }
}

void ATATExclusiveToggleGroup::_OnToggleStateChanged(bool is_on)
{
   _UpdateActiveToggle();
}

const AOSESyncedToggle* ATATExclusiveToggleGroup::_FindActiveToggle() const
{
   for(const AOSESyncedToggle* toggle : _toggles)
   {
      if(toggle && toggle->IsOn())
      {
         return toggle;
      }
   }

   return nullptr;
}

void ATATExclusiveToggleGroup::_UpdateActiveToggle()
{
   const AOSESyncedToggle* newToggle = _FindActiveToggle();
   if(newToggle != _activeToggle)
   {
      _activeToggle = newToggle;
      OnActiveToggleChanged.Broadcast();
   }
}

void UTATExclusiveToggleRequirementComponent::BeginPlay()
{
   Super::BeginPlay();

   if(_toggleGroup == nullptr)
   {
      return;
   }

   if(AOSESyncedToggle* toggle = GetOwner<AOSESyncedToggle>())
   {
      _toggleGroup->RegisterToggle(toggle);
      _toggleGroup->OnActiveToggleChanged.AddUObject(this, &ThisClass::_OnActiveToggleChanged);
   }
}

bool UTATExclusiveToggleRequirementComponent::IsInteractionBlocked() const
{
   return IsOtherToggleActive();
}

void UTATExclusiveToggleRequirementComponent::AddToPrompt(FInteractPrompt& prompt) const
{
   prompt.ErrorMessage = _blockedMessage;
}

bool UTATExclusiveToggleRequirementComponent::IsOtherToggleActive() const
{
   return _isOtherToggleActive;
}

#if WITH_EDITOR
void UTATExclusiveToggleRequirementComponent::OnRegister()
{
   Super::OnRegister();
   _RefreshEditorVisToggleGroup();
}

void UTATExclusiveToggleRequirementComponent::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   const FName propertyName = (propertyChangedEvent.Property != nullptr) ? propertyChangedEvent.Property->GetFName() : NAME_None;

   if (propertyName == GET_MEMBER_NAME_CHECKED(UTATExclusiveToggleRequirementComponent, _toggleGroup))
   {
      _RefreshEditorVisToggleGroup();
   }
}

void UTATExclusiveToggleRequirementComponent::_RefreshEditorVisToggleGroup()
{
   if (_editorVisGroup)
   {
      _editorVisGroup->RemoveEditorVisToggle(GetOwner());
   }
   _editorVisGroup = _toggleGroup;
   if (_editorVisGroup)
   {
      _editorVisGroup->AddEditorVisToggle(GetOwner());
   }
}
#endif // WITH_EDITOR

void UTATExclusiveToggleRequirementComponent::_OnActiveToggleChanged()
{
   check(_toggleGroup);
   const AOSESyncedToggle* activeToggle = _toggleGroup->GetActiveToggle();
   const bool otherToggleActive = activeToggle && activeToggle != GetOwner();
   if(otherToggleActive != _isOtherToggleActive)
   {
      _isOtherToggleActive = otherToggleActive;
      OnOtherToggleActiveChanged.Broadcast(_isOtherToggleActive);
   }
}

UTATExclusiveToggleGroupVisComponent::UTATExclusiveToggleGroupVisComponent()
{
   bIsEditorOnly = true;
   bEditableWhenInherited = false;
#if WITH_EDITORONLY_DATA
   SetIsVisualizationComponent(true);
#endif
}

