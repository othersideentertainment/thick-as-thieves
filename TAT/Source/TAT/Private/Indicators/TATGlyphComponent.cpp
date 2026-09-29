// (c) 2018-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "Indicators/TATGlyphComponent.h"

// ue
#include "DrawDebugHelpers.h"
#include "GameFramework/GameStateBase.h"
#include "GameFramework/PlayerState.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Misc/DataValidation.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATGlyphComponent)
DEFINE_LOG_CATEGORY_STATIC(LogTATGlyphComponent, Log, All);

TAutoConsoleVariable<int32> CVarTATGlyphDebug(
   TEXT("TAT.Glyph.DebugDraw"),
   0,
   TEXT("Whether to debug draw all glyphs in the world\n")
   TEXT(" 0 (default) - Don't show glyph debug data\n")
   TEXT(" 1 - Show all glyph debug data\n")
);

namespace GlyphHelpers
{
   static APawn* FindLocalPlayerPawn(UWorld* world)
   {
      if (world == nullptr)
      {
         return nullptr;
      }

      AGameStateBase* gameState = world->GetGameState();
      if (gameState == nullptr)
      {
         return nullptr;
      }

      for (const TObjectPtr<APlayerState>& player : gameState->PlayerArray)
      {
         if (player)
         {
            APlayerController* pc = player->GetPlayerController();
            if (pc != nullptr && pc->IsLocalController())
            {
               if (APawn* pawn = pc->GetPawnOrSpectator())
               {
                  return pawn;
               }
            }
         }
      }

      return nullptr;
   }
}

UTATGlyphComponent::UTATGlyphComponent(const FObjectInitializer& objectInitializer)
{
   // Component should only tick when animating the glyph opacity
   PrimaryComponentTick.bCanEverTick = true;
   PrimaryComponentTick.bStartWithTickEnabled = false;
}

#if WITH_EDITOR
EDataValidationResult UTATGlyphComponent::IsDataValid(FDataValidationContext& context) const
{
   EDataValidationResult result = Super::IsDataValid(context);

   // Ensure a glyph material is assigned
   const UMaterialInterface* glyphMaterial = _glyphMaterialElement.Material;
   if (!IsValid(glyphMaterial))
   {
      context.AddError(FText::FromString(FString::Printf(TEXT("UTATGlyphComponent has unassigned material in _glyphMaterialElement! Please assign a material"))));
   }

   auto validateParamName = [&context, glyphMaterial](const FString& memberName, FName paramName, EMaterialParameterType paramType) {
      if (!paramName.IsValid())
      {
         context.AddError(FText::FromString(FString::Printf(TEXT("UTATGlyphComponent has unassigned %s param!"), *memberName)));
      }
      else
      {
         FMaterialParameterMetadata paramData;
         if (glyphMaterial && !glyphMaterial->GetParameterDefaultValue(paramType, paramName, paramData))
         {
            context.AddError(FText::FromString(FString::Printf(TEXT("UTATGlyphComponent has %s = %s that doesn't correspond to a valid parameter of material %s!")
               , *memberName
               , *paramName.ToString()
               , *glyphMaterial->GetName())));
         }
      }
   };

   // Validate glyph texture params
   validateParamName(GET_MEMBER_NAME_STRING_CHECKED(UTATGlyphComponent, _materialGlyphTextureParamName), _materialGlyphTextureParamName, EMaterialParameterType::Texture);

   // Validate primary/secondary color params
   validateParamName(GET_MEMBER_NAME_STRING_CHECKED(UTATGlyphComponent, _materialGlyphPrimaryColorParamName), _materialGlyphPrimaryColorParamName, EMaterialParameterType::Vector);
   validateParamName(GET_MEMBER_NAME_STRING_CHECKED(UTATGlyphComponent, _materialGlyphSecondaryColorParamName), _materialGlyphSecondaryColorParamName, EMaterialParameterType::Vector);

   // Validate material opacity param
   validateParamName(GET_MEMBER_NAME_STRING_CHECKED(UTATGlyphComponent, _materialOpacityParamName), _materialOpacityParamName, EMaterialParameterType::Scalar);

   if (context.GetNumErrors() + context.GetNumWarnings() > 0)
   {
      result = EDataValidationResult::Invalid;
   }
   return result;
}
#endif // WITH_EDITOR

void UTATGlyphComponent::OnRegister()
{
   Super::OnRegister();
   
   if (_glyphMaterialElement.Material)
   {
      TArray<FMaterialSpriteElement> glyphMaterialElements;
      glyphMaterialElements.Add(_glyphMaterialElement);

      SetElements(glyphMaterialElements);

      if (!IsValid(_glyphDynamicMaterialInstance))
      {
         _glyphDynamicMaterialInstance = _ConstructDynamicMaterialInstance();
      }
   }

#if OSE_CHEATS_ENABLED
   if (_enableAutoDrawDebugGlyphTick && GetWorld()->GetNetMode() != NM_DedicatedServer)
   {
      // Automatically toggle the debug draw tick function when the glyph debug cvar changes
      if (!_debugCvarChangedHandle.IsValid())
      {
         _debugCvarChangedHandle = CVarTATGlyphDebug->OnChangedDelegate().AddUObject(this, &UTATGlyphComponent::_OnDebugCvarChanged);
      }

      // Enable or disable debug draw right now based on the current value of the cvar
      if (IConsoleVariable* cvar = CVarTATGlyphDebug.AsVariable())
      {
         _OnDebugCvarChanged(cvar);
      }
   }
#endif
}

void UTATGlyphComponent::OnUnregister()
{
#if OSE_CHEATS_ENABLED
   if (_debugDrawTickHandle.IsValid())
   {
      FTSTicker::GetCoreTicker().RemoveTicker(_debugDrawTickHandle);
      _debugDrawTickHandle.Reset();
   }
#endif

   Super::OnUnregister();
}

void UTATGlyphComponent::TickComponent(float deltaTime, ELevelTick tickType, FActorComponentTickFunction* thisTickFunction)
{
   Super::TickComponent(deltaTime, tickType, thisTickFunction);

   check(IsValid(_glyphDynamicMaterialInstance));

   float currentOpacity = -1.f;
   if (!_glyphDynamicMaterialInstance->GetScalarParameterValue(_materialOpacityParamName, currentOpacity))
   {
      UE_LOG(LogTATGlyphComponent, Error, TEXT("[%s] | Failed to query opacity scalar parameter value! Ensure that the parameter data in _materialOpacityParam matches the material's opacity parameter")
         , *GetOwner()->GetName());
      PrimaryComponentTick.SetTickFunctionEnable(false);
      return;
   }

   // Increase/decrease current opacity towards target
   float delta = deltaTime * (1.f / FMath::Max(_opacityFadeSeconds, 0.1f));
   if (!_isVisible)
   {
      delta *= -1.f;
   }
   currentOpacity = FMath::Clamp(currentOpacity + delta, 0.f, 1.f);
   _glyphDynamicMaterialInstance->SetScalarParameterValue(_materialOpacityParamName, currentOpacity);

   // Disable tick if we've reached our target opacity
   const float targetOpacity = _isVisible ? 1.f : 0.f;
   if (currentOpacity == targetOpacity)
   {
      UE_LOG(LogTATGlyphComponent, Verbose, TEXT("[%s] | Reached target opacity %f! Disabling tick..."), *GetOwner()->GetName(), currentOpacity);
      PrimaryComponentTick.SetTickFunctionEnable(false);
   }

   // Disable rendering when opacity reaches 0 (enable otherwise)
   const bool isVisible = currentOpacity != 0.f;
   SetVisibility(isVisible);
}

void UTATGlyphComponent::SetGlyphTexture(UTexture* glyphTexture)
{
   if (!IsValid(glyphTexture))
   {
      UE_LOG(LogTATGlyphComponent, Warning, TEXT("[%s] SetGlyphTexture() called with invalid glyphTexture param!"), *GetOwner()->GetName());
      return;
   }
   _glyphDynamicMaterialInstance = GetOrCreateDynamicMaterialInstance();
   if (_glyphDynamicMaterialInstance)
   {
      _glyphDynamicMaterialInstance->SetTextureParameterValue(_materialGlyphTextureParamName, glyphTexture);
   }
   else
   {
      UE_LOG(LogTATGlyphComponent, Warning, TEXT("[%s] SetGlyphTexture() failed due to invalid _glyphDynamicMaterialInstance!"), *GetOwner()->GetName());
   }
}

void UTATGlyphComponent::SetGlyphColors(FLinearColor primaryColor, FLinearColor secondaryColor)
{
   // Construct material instance if needed
   if (!IsValid(_glyphDynamicMaterialInstance))
   {
      _glyphDynamicMaterialInstance = _ConstructDynamicMaterialInstance();
   }
   if (_glyphDynamicMaterialInstance)
   {
      _glyphDynamicMaterialInstance->SetVectorParameterValue(_materialGlyphPrimaryColorParamName, primaryColor);
      _glyphDynamicMaterialInstance->SetVectorParameterValue(_materialGlyphSecondaryColorParamName, secondaryColor);
   }
   else
   {
      UE_LOG(LogTATGlyphComponent, Warning, TEXT("[%s] SetGlyphColors() failed due to invalid _glyphDynamicMaterialInstance!"), *GetOwner()->GetName());
   }
}

UMaterialInstanceDynamic* UTATGlyphComponent::GetOrCreateDynamicMaterialInstance()
{
   if (!IsValid(_glyphDynamicMaterialInstance))
   {
      _glyphDynamicMaterialInstance = _ConstructDynamicMaterialInstance();
   }
   return _glyphDynamicMaterialInstance;
}

bool UTATGlyphComponent::GetGlyphColorParamDefaultValues(FLinearColor& outPrimaryColor, FLinearColor& outSecondaryColor) const
{
   const UMaterialInterface* glyphMaterial = _glyphMaterialElement.Material;
   if (!glyphMaterial)
   {
      UE_LOG(LogTATGlyphComponent, Warning, TEXT("GetGlyphColorParamDefaultValues() failed due to unassigned glyph material!"));
      return false;
   }

   if (!glyphMaterial->GetVectorParameterDefaultValue(_materialGlyphPrimaryColorParamName, outPrimaryColor))
   {
      UE_LOG(LogTATGlyphComponent, Warning, TEXT("GetGlyphColorParamDefaultValues() failed due to invalid _materialGlyphPrimaryColorParamName (%s)!"), *_materialGlyphPrimaryColorParamName.ToString());
      return false;
   }
   if (!glyphMaterial->GetVectorParameterDefaultValue(_materialGlyphSecondaryColorParamName, outSecondaryColor))
   {
      UE_LOG(LogTATGlyphComponent, Warning, TEXT("GetGlyphColorParamDefaultValues() failed due to invalid _materialGlyphSecondaryColorParamName (%s)!"), *_materialGlyphSecondaryColorParamName.ToString());
      return false;
   }

   return true;
}

bool UTATGlyphComponent::GetGlyphTextureDefaultValue(UTexture*& outTexture) const
{
   const UMaterialInterface* glyphMaterial = _glyphMaterialElement.Material;
   if (!glyphMaterial)
   {
      UE_LOG(LogTATGlyphComponent, Warning, TEXT("GetGlyphTextureDefaultValue() failed due to unassigned glyph material!"));
      return false;
   }
   const bool textureFound = glyphMaterial->GetTextureParameterDefaultValue(_materialGlyphTextureParamName, outTexture);
   UE_CLOG(!textureFound, LogTATGlyphComponent, Warning, TEXT("GetGlyphTextureDefaultValue() failed due to invalid _materialGlyphTextureParamName (%s)!"), *_materialGlyphPrimaryColorParamName.ToString());
   return textureFound;
}

// static
bool UTATGlyphComponent::IsGlyphDebugModeEnabled()
{
#if OSE_CHEATS_ENABLED
   check(IsInGameThread());
   const int32 glyphDebuggingCvar = CVarTATGlyphDebug.GetValueOnGameThread();
   return glyphDebuggingCvar != 0;
#else
   return false;
#endif
}

// static
void UTATGlyphComponent::SetGlyphDebugMode(bool enabled, bool setFromCheat)
{
#if OSE_CHEATS_ENABLED
   if (IConsoleVariable* cvar = CVarTATGlyphDebug.AsVariable())
   {
      const EConsoleVariableFlags cvarSetBy = setFromCheat ? ECVF_SetByConsole : ECVF_SetByCode;
      cvar->Set(enabled ? 1 : 0, cvarSetBy);
   }
#endif
}

void UTATGlyphComponent::SetGlyphVisibility(bool newVisible)
{
   if (newVisible != _isVisible)
   {
      _SetGlyphVisible(newVisible);
   }
}

void UTATGlyphComponent::_SetGlyphVisible(bool shouldBeVisible, bool instant)
{
   // Update visibility
   _isVisible = shouldBeVisible;

   // Construct glyph material instance if haven't already
   if (!IsValid(_glyphDynamicMaterialInstance))
   {
      _glyphDynamicMaterialInstance = GetOrCreateDynamicMaterialInstance();
      if (!_glyphDynamicMaterialInstance)
      {
         UE_LOG(LogTATGlyphComponent, Error, TEXT("[%s] Failed to construct dynamic material instance!"), *GetOwner()->GetName());
         PrimaryComponentTick.SetTickFunctionEnable(false);
         return;
      }
   }

   if (instant)
   {
      _glyphDynamicMaterialInstance->SetScalarParameterValue(_materialOpacityParamName, shouldBeVisible ? 1.0f : 0.0f);

      // Only render the component if we're supposed to be visible
      SetVisibility(shouldBeVisible);

      // No need to animate opacity, so keep tick disabled
      PrimaryComponentTick.SetTickFunctionEnable(false);
   }
   else
   {
      UE_LOG(LogTATGlyphComponent, Verbose, TEXT("[%s] animating visibility to %s")
         , *GetOwner()->GetName()
         , shouldBeVisible ? TEXT("visible") : TEXT("invisible"));

      // Enable tick to animate opacity
      PrimaryComponentTick.SetTickFunctionEnable(true);
   }

   // Notify
   OnGlyphVisibilityChanged.Broadcast(shouldBeVisible);
}

UMaterialInstanceDynamic* UTATGlyphComponent::_ConstructDynamicMaterialInstance()
{
   // Make sure we haven't already constructed a material instance
   check(!IsValid(_glyphDynamicMaterialInstance));

   if (!IsValid(_glyphMaterialElement.Material))
   {
      UE_LOG(LogTATGlyphComponent, Warning, TEXT("[%s] Unassigned glyph material! Could not construct dynamic material instance"), *GetOwner()->GetName());
      return nullptr;
   }

   UE_LOG(LogTATGlyphComponent, Verbose, TEXT("[%s] | Constructing dynamic material instance with glyph material %s...")
      , *GetOwner()->GetName()
      , *_glyphMaterialElement.Material->GetName());

   UMaterialInstanceDynamic* dynamicMaterialInstance = CreateDynamicMaterialInstance(0, _glyphMaterialElement.Material);
   check(dynamicMaterialInstance != nullptr);

   // Init opacity to 0, so it can fade into visibility (rather than snapping on)
   dynamicMaterialInstance->SetScalarParameterValue(_materialOpacityParamName, 0.0f);

   return dynamicMaterialInstance;
}

void UTATGlyphComponent::_DrawDebugGlyph(const FVector& playerLocation)
{
#if OSE_CHEATS_ENABLED
   if (!IsGlyphDebugModeEnabled())
   {
      return;
   }

   UWorld* world = GetWorld();
   if (world == nullptr)
   {
      return;
   }

   static constexpr bool persistentLines = false;
   static constexpr float drawDuration = 0.0f;

   const FBoxSphereBounds glyphBounds = CalcBounds(GetComponentTransform());
   const FColor glyphDebugColor = FColor::Cyan;

   // Draw the glyph bounds if the glyph is visible, otherwise draw it as a point that's visible through walls
   if (_isVisible)
   {
      static constexpr int32 boundsSphereNumSegments = 6;
      DrawDebugSphere(world, glyphBounds.Origin, glyphBounds.SphereRadius, boundsSphereNumSegments, glyphDebugColor, persistentLines, drawDuration);
   }
   else
   {
      // Scale the point size based on the distance from the glyph to the player
      const float pointSize = FMath::GetMappedRangeValueClamped<float>(
         FVector2f(1000.0f, 8000.0f),
         FVector2f(12.0f, 2.0f),
         FVector::Dist(playerLocation, glyphBounds.Origin));

      DrawDebugPoint(world, glyphBounds.Origin, pointSize, glyphDebugColor, persistentLines, drawDuration, SDPG_Foreground);
   }
#endif
}

#if OSE_CHEATS_ENABLED
void UTATGlyphComponent::_OnDebugCvarChanged(IConsoleVariable* cvar)
{
   check(cvar != nullptr);
   const int32 glyphDebuggingCvar = cvar->GetInt();
   const bool glyphDebugEnabled = glyphDebuggingCvar != 0;
   if (glyphDebugEnabled)
   {
      if (!_debugDrawTickHandle.IsValid())
      {
         _debugDrawTickHandle = FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateUObject(this, &UTATGlyphComponent::_AutoDrawDebugGlyphTick));
      }
   }
   else
   {
      if (_debugDrawTickHandle.IsValid())
      {
         FTSTicker::GetCoreTicker().RemoveTicker(_debugDrawTickHandle);
         _debugDrawTickHandle.Reset();
      }
   }
}

bool UTATGlyphComponent::_AutoDrawDebugGlyphTick(float worldDeltaSeconds)
{
   // Ticker delegates should return true to automatically reschedule at the same delay, or false for a one-shot.
   constexpr bool continueTicking = true;

   if (IsGlyphDebugModeEnabled())
   {
      if (APawn* localPlayerPawn = GlyphHelpers::FindLocalPlayerPawn(GetWorld()))
      {
         FVector loc;
         FRotator rot;
         localPlayerPawn->GetActorEyesViewPoint(loc, rot);
         _DrawDebugGlyph(loc);
      }
   }

   return continueTicking;
}
#endif
