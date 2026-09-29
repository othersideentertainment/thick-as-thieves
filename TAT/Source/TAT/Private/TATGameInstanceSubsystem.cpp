// (c) 2025 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "TATGameInstanceSubsystem.h"
#include "TATGameInstance.h"

//+jmb: DEPRECATED - WILL BE REMOVING SOON

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGameInstanceSubsystem)


void UTATGameInstanceSubsystem::Initialize(FSubsystemCollectionBase& collection)
{
    Super::Initialize(collection);

    _TATGameInstance = Cast<UTATGameInstance>(GetGameInstance());

    _InitializeSingleton();

    if (_TATGameInstance)
    {
        /*
        _TATGameInstance->WorldBeginPlayEvent.AddWeakLambda(this, [this](UWorld*)
            {
                SetupBindings();
            });
        */
    }
}

void UTATGameInstanceSubsystem::Deinitialize()
{
    Super::Deinitialize();

    _DeinitializeSingleton();
}

#if 0
void UTATGameInstanceSubsystem::SetupBindings()
{
    auto* CohtmlWidget = UTheMetaUiFunctionLibrary::GetCohtmlWidget(this);
    if (ensureMsgf(CohtmlWidget, TEXT("Failed to retrieve internal Cohtml Widget")))
    {
        if (CohtmlWidget->IsReadyForBindings())
        {
            InitializeCoherentBindings();
        }

        CohtmlWidget->ReadyForBindings.AddDynamic(this, &UTATGameInstanceSubsystem::InitializeCoherentBindings);
        CohtmlWidget->BindingsReleased.AddDynamic(this, &UTATGameInstanceSubsystem::ReleaseCoherentBindings);
    }
}
#endif

//-jmb: DEPRECATED - WILL BE REMOVING SOON
