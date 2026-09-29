// (c) 2018-2023 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#include "WorldMap/TATWorldMapBoundary.h"

// tat
#include "Player/TATPlayerController.h"
#include "WorldMap/TATWorldMapSubsystem.h"
#include "Developer/TATProjectSettings.h"

// ue
#include "Components/BoxComponent.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Engine/TextureRenderTarget2D.h"
#include "GameFramework/Character.h"
#include "ImageUtils.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(TATWorldMapBoundary)

DEFINE_LOG_CATEGORY_STATIC(LogTATWorldMapBoundary, Log, All);

DECLARE_STATS_GROUP(TEXT("TAT World Map"), STATGROUP_WorldMap, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("TAT World Map: Update Map Render Target Texture"), STAT_WorldMapBoundary_UpdateMapRenderTargetTexture, STATGROUP_WorldMap);

FVector FTATWorldMapAutoSwitchBounds::GetRelativeLocation(const FVector& scale) const
{
   return OriginOffset / FVector(FMath::Max(0.001f, scale.X), FMath::Max(0.001f, scale.Y), FMath::Max(0.001f, scale.Z));
}

FVector FTATWorldMapAutoSwitchBounds::GetRelativeExtent(const FVector& scale, float aspectRatio) const
{
   FVector result = Extent;
   if (UseRelativeExtent)
   {
      result *= FVector(100, 100, 100);

      if (ApplyAspectRatio)
      {
         if (aspectRatio > 1.0f)
         {
            result.Y *= 1.0f / aspectRatio;
         }
         else
         {
            result.X *= aspectRatio;
         }
      }
   }
   else
   {
      result /= FVector(FMath::Max(0.001f, scale.X), FMath::Max(0.001f, scale.Y), FMath::Max(0.001f, scale.Z));
   }
   return result;
}

ATATWorldMapBoundary::ATATWorldMapBoundary(const FObjectInitializer& objectInitializer)
{
   // Create boundary box with zero height
   _boxComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxComponent"));
   _boxComponent->SetBoxExtent(FVector(100, 100, 0));
   _boxComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   _boxComponent->SetMobility(EComponentMobility::Static);

   _autoSwitchOverlapComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("AutoSwitchOverlapComponent"));
   _autoSwitchOverlapComponent->SetupAttachment(_boxComponent);
   _autoSwitchOverlapComponent->SetRelativeLocation(FVector::ZeroVector);
   _autoSwitchOverlapComponent->SetBoxExtent(FVector(1, 1, 1));
   _autoSwitchOverlapComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
   _autoSwitchOverlapComponent->SetMobility(EComponentMobility::Static);
   _autoSwitchOverlapComponent->ShapeColor = FColor(75, 255, 150, 255);

#if WITH_EDITORONLY_DATA
   _mapBoundsPreviewComponent = CreateEditorOnlyDefaultSubobject<UBoxComponent>(TEXT("MapBoundsPreviewComponent"));
   if (_mapBoundsPreviewComponent)
   {
      _mapBoundsPreviewComponent->SetupAttachment(_boxComponent);
      _mapBoundsPreviewComponent->SetBoxExtent(FVector(100, 100, 0));
      _mapBoundsPreviewComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
      _mapBoundsPreviewComponent->SetMobility(EComponentMobility::Static);
      _mapBoundsPreviewComponent->ShapeColor = FColor(243, 169, 177, 255);;
   }
#endif

   _mapSceneCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MapSceneCaptureComponent"));
   _mapSceneCaptureComponent->SetupAttachment(RootComponent);
   _mapSceneCaptureComponent->SetRelativeLocation(FVector(0, 0, 50));
   _mapSceneCaptureComponent->SetRelativeRotation(FRotator(0, -90, 0));
   _mapSceneCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
   _mapSceneCaptureComponent->OrthoWidth = 32000.0f;
   _mapSceneCaptureComponent->CaptureSource = MapRenderTextureCaptureSource;
   _mapSceneCaptureComponent->bCaptureEveryFrame = false;
   _mapSceneCaptureComponent->bAlwaysPersistRenderingState = true;

   // Apply editor line thickness
#if WITH_EDITOR
   _boxComponent->SetLineThickness(_editorLineThickness);
   _autoSwitchOverlapComponent->SetLineThickness(_editorLineThickness);
   if (_mapBoundsPreviewComponent)
   {
      _mapBoundsPreviewComponent->SetLineThickness(FMath::Max(0.0f, _editorLineThickness * 0.5f));
   }
#endif // WITH_EDITOR
}

void ATATWorldMapBoundary::OnConstruction(const FTransform& transform)
{
   Super::OnConstruction(transform);

   check(IsValid(_boxComponent));
   const bool nonPrimaryMap = MapType != ETATWorldMapBoundaryType::Primary;
   _boxComponent->SetBoxExtent(FVector(100, 100, nonPrimaryMap ? 100 : 0));
   if (nonPrimaryMap && AutoSwitchToMap)
   {
      _autoSwitchOverlapComponent->SetVisibility(true);
      _autoSwitchOverlapComponent->SetRelativeLocation(AutoSwitchBounds.GetRelativeLocation(GetActorScale3D()));
      _autoSwitchOverlapComponent->SetBoxExtent(AutoSwitchBounds.GetRelativeExtent(GetActorScale3D(), GetMapAspectRatio()));
      _autoSwitchOverlapComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
      _autoSwitchOverlapComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
      _autoSwitchOverlapComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
   }
   else
   {
      _autoSwitchOverlapComponent->SetVisibility(false);
      _autoSwitchOverlapComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
   }

#if WITH_EDITORONLY_DATA
   if (_mapBoundsPreviewComponent)
   {
      _mapBoundsPreviewComponent->SetBoxExtent(GetMapAreaBoundingBox().GetExtent());
      _mapBoundsPreviewComponent->SetRelativeLocation(FVector::Zero());
      _mapBoundsPreviewComponent->SetWorldScale3D(FVector::One());
   }
#endif
}

void ATATWorldMapBoundary::BeginPlay()
{
   Super::BeginPlay();

   if (UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
   {
      worldMapSubsystem->RegisterMapBoundary(this);
   }

   if (MapType != ETATWorldMapBoundaryType::Primary && AutoSwitchToMap)
   {
      _autoSwitchOverlapComponent->OnComponentBeginOverlap.AddDynamic(this, &ATATWorldMapBoundary::_OnBoxBeginOverlap);
      _autoSwitchOverlapComponent->OnComponentEndOverlap.AddDynamic(this, &ATATWorldMapBoundary::_OnBoxEndOverlap);
   }
}

void ATATWorldMapBoundary::EndPlay(const EEndPlayReason::Type endPlayReason)
{
   if (UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
   {
      worldMapSubsystem->UnregisterMapBoundary(this);
   }

   Super::EndPlay(endPlayReason);
}

FVector2D ATATWorldMapBoundary::GetUnscaledBoundaryExtents() const
{
   check(IsValid(_boxComponent));
   return FVector2D(_boxComponent->GetUnscaledBoxExtent());
}

bool ATATWorldMapBoundary::UseAutoGeneratedMap() const
{
   return ForceAutoGenerateMap || (AutoGenerateMapIfMissingMapMaterial && MapMaterial.IsNull());
}

UTexture* ATATWorldMapBoundary::GetAutoGeneratedMapTexture(bool forceUpdate)
{
   if (!UseAutoGeneratedMap())
   {
      return nullptr;
   }
   _UpdateMapTextureUsingRenderTarget(forceUpdate);
   return _mapRenderTarget;
}

TSoftObjectPtr<UMaterialInterface> ATATWorldMapBoundary::GetMapMaterial() const
{
   return UseAutoGeneratedMap() ? UTATProjectSettings::Get().AutoGeneratedMapMaterial : MapMaterial;
}

void ATATWorldMapBoundary::GetMapTextureAnchorPoints(FVector2D& outTopLeft, FVector2D& outBottomRight, bool normalized) const
{
   if (UseAutoGeneratedMap() && UseAutoGeneratedMapAnchors)
   {
      outTopLeft = AutoGeneratedMapAnchorTopLeft;
      outBottomRight = AutoGeneratedMapAnchorBottomRight;
   }
   else
   {
      outTopLeft = MapTextureAnchorTopLeft;
      outBottomRight = MapTextureAnchorBottomRight;
   }

   if (normalized)
   {
      outTopLeft /= FVector2D(2048.0f);
      outBottomRight /= FVector2D(2048.0f);
   }
}

FVector2D ATATWorldMapBoundary::GetMapAreaRelativeSize() const
{
   constexpr bool normalized = true;
   FVector2D topLeft = FVector2D::ZeroVector;
   FVector2D bottomRight = FVector2D::ZeroVector;
   GetMapTextureAnchorPoints(topLeft, bottomRight, normalized);
   return bottomRight - topLeft;
}

float ATATWorldMapBoundary::GetMapAspectRatio() const
{
   const FVector2D mapAreaRelativeSize = GetMapAreaRelativeSize();
   return !mapAreaRelativeSize.IsNearlyZero() ? (mapAreaRelativeSize.X / mapAreaRelativeSize.Y) : 1.0f;
}

FBox ATATWorldMapBoundary::GetMapAreaBoundingBox() const
{
   check(_boxComponent != nullptr);

   const FVector2D mapAreaRelativeSize = GetMapAreaRelativeSize();
   FVector boxExtent = _boxComponent->GetScaledBoxExtent();
   if (!mapAreaRelativeSize.IsNearlyZero())
   {
      if (GetMapAspectRatio() > 1.0f)
      {
         boxExtent.Y *= mapAreaRelativeSize.Y / mapAreaRelativeSize.X;
      }
      else
      {
         boxExtent.X *= mapAreaRelativeSize.X / mapAreaRelativeSize.Y;
      }
   }

   const FVector baseLocation = _boxComponent->GetComponentLocation();
   const FBox box{ baseLocation - boxExtent, baseLocation + boxExtent };
   return box;
}

#if WITH_EDITOR
void ATATWorldMapBoundary::PostEditChangeProperty(FPropertyChangedEvent& propertyChangedEvent)
{
   Super::PostEditChangeProperty(propertyChangedEvent);

   FName propertyName = (propertyChangedEvent.Property != nullptr) ? propertyChangedEvent.Property->GetFName() : NAME_None;
   if (propertyName == GET_MEMBER_NAME_CHECKED(ATATWorldMapBoundary, _editorLineThickness))
   {
      // Refresh line thickness in case it was modified
      _boxComponent->SetLineThickness(_editorLineThickness);
      _autoSwitchOverlapComponent->SetLineThickness(_editorLineThickness);
      if (_mapBoundsPreviewComponent)
      {
         _mapBoundsPreviewComponent->SetLineThickness(FMath::Max(0.0f, _editorLineThickness * 0.5f));
      }
   }
}

void ATATWorldMapBoundary::UpdateMapRenderTarget()
{
   constexpr bool forceUpdate = true;
   _UpdateMapTextureUsingRenderTarget(forceUpdate);
}

void ATATWorldMapBoundary::ExportAutoGeneratedMapToImage()
{
   UpdateMapRenderTarget();

   if (_mapRenderTarget == nullptr)
   {
      UE_LOG(LogTATWorldMapBoundary, Error, TEXT("Render target export failed: render target asset is invalid"));
      return;
   }

   FString fileName = ExportImageFileName;
   if (fileName.IsEmpty())
   {
      fileName = !MapLabel.IsEmpty() ? MapLabel.ToString() : *GetNameSafe(this);
   }
   auto isValidFileNameChar = [](TCHAR ch) -> bool
   {
      return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z') || (ch >= '0' && ch <= '9') || ch == '_' || ch == '-' || ch == '.';
   };
   for (int32 i = 0; i < fileName.Len(); i++)
   {
      if (!isValidFileNameChar(fileName[i]))
      {
         fileName[i] = '_';
      }
   }
   if (!fileName.EndsWith(TEXT(".png"), ESearchCase::IgnoreCase))
   {
      fileName.Append(TEXT(".png"));
   }

   auto exportRenderTargetToPNG = [](UTextureRenderTarget* rt, const TCHAR* filename, float aspectRatio, const TOptional<TFunctionRef<void(FColor&)>>& pixelCallback)
   {
      check(rt != nullptr);
      FImage img;
      if (!FImageUtils::GetRenderTargetImage(rt, img))
      {
         UE_LOG(LogTATWorldMapBoundary, Error, TEXT("Render target export failed: failed get render target image"));
         return false;
      }

      // Ensure the format is BGRA8 (same as FColor)
      if (img.Format != ERawImageFormat::Type::BGRA8)
      {
         img.ChangeFormat(ERawImageFormat::Type::BGRA8, EGammaSpace::Linear);
      }

      if (pixelCallback)
      {
         const auto& callback = pixelCallback.GetValue();
         for (int32 y = 0; y < img.GetHeight(); y++)
         {
            for (int32 x = 0; x < img.GetWidth(); x++)
            {
               callback(*(static_cast<FColor*>(img.GetPixelPointer(x, y))));
            }
         }
      }

      TArray64<uint8> compressedData;
      if (aspectRatio != 1.0f)
      {
         FImage resizedImg;
         const int32 newHeight = FMath::RoundToInt((float)img.GetHeight() * (1.0f / aspectRatio));
         img.ResizeTo(resizedImg, img.GetWidth(), newHeight, ERawImageFormat::BGRA8, EGammaSpace::Linear);
         if (!FImageUtils::CompressImage(compressedData, TEXT("PNG"), resizedImg))
         {
            UE_LOG(LogTATWorldMapBoundary, Error, TEXT("Render target export failed: failed to compress resized image"));
            return false;
         }
      }
      else
      {
         if (!FImageUtils::CompressImage(compressedData, TEXT("PNG"), img))
         {
            UE_LOG(LogTATWorldMapBoundary, Error, TEXT("Render target export failed: failed to compress image"));
            return false;
         }
      }

      return FFileHelper::SaveArrayToFile(compressedData, filename);
   };

   // Change the format so the image is saved correctly
   const ETextureRenderTargetFormat origFormat = _mapRenderTarget->RenderTargetFormat;
   _mapRenderTarget->RenderTargetFormat = RTF_RGBA8;
   UpdateMapRenderTarget();

   FString fullPath = FPaths::ProjectSavedDir() / TEXT("WorldMapBoundary");
   if (UWorld* world = GetWorld())
   {
      const FString mapName = world->GetMapName();
      if (!mapName.IsEmpty())
      {
         fullPath /= mapName;
      }
   }
   fullPath /= fileName;

   // This is so, so stupid. When you export a render target, the alpha channel is flipped, so it exports "correctly" but shows up as a fully
   // transparent image in any image editing program.
   TOptional<TFunctionRef<void(FColor&)>> pixelCallback;
   if (ExportImage_RemoveAlphaChannel)
   {
      // Remove the alpha channel entirely
      pixelCallback = [](FColor& px)
      {
         px.A = 255;
      };
   }
   else if (ExportImage_InvertAlphaChannel)
   {
      // Flip the alpha channel
      pixelCallback = [](FColor& px)
      {
         px.A = 255 - px.A;
      };
   }

   if (!exportRenderTargetToPNG(_mapRenderTarget, *fullPath, GetMapAspectRatio(), pixelCallback))
   {
      UE_LOG(LogTATWorldMapBoundary, Error, TEXT("Render target export failed: failed to compress image or save file to location: %s"), *fullPath);
   }

   // Restore format
   _mapRenderTarget->RenderTargetFormat = origFormat;
   UpdateMapRenderTarget();

   UE_LOG(LogTATWorldMapBoundary, Log, TEXT("Render target exported: %s"), *IFileManager::Get().ConvertToAbsolutePathForExternalAppForRead(*fullPath));
}
#endif // WITH_EDITOR

void ATATWorldMapBoundary::_UpdateMapTextureUsingRenderTarget(bool forceUpdate)
{
   if (_mapRenderTarget == nullptr)
   {
      if (UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
      {
         TSoftObjectPtr<UTextureRenderTarget2D> renderTarget = worldMapSubsystem->GetRenderTargetForAutoGeneratedMap(this);
         if (!renderTarget.IsNull())
         {
            _mapRenderTarget = renderTarget.LoadSynchronous();
         }
      }
#if WITH_EDITOR
      else
      {
         // If this in an editor context, just grab a render target for preview purposes
         if (MapType == ETATWorldMapBoundaryType::Primary)
         {
            _mapRenderTarget = UTATProjectSettings::Get().AutoGeneratedMapRenderTarget.LoadSynchronous();
         }
         else
         {
            _mapRenderTarget = UTATProjectSettings::Get().AutoGeneratedMapRenderTargets_Secondary[0].LoadSynchronous();
         }
      }
#endif
   }

   if (_mapRenderTarget == nullptr)
   {
      return;
   }

   bool updatedRenderTarget = false;

   const FBox mapAreaBounds = GetMapAreaBoundingBox();

   // If we've already captured the map texture with the render target, don't capture again unless forceUpdate is true
   if (forceUpdate || !_mapRenderTargetCaptured)
   {
      SCOPE_CYCLE_COUNTER(STAT_WorldMapBoundary_UpdateMapRenderTargetTexture);

      UE_LOG(LogTATWorldMapBoundary, Log, TEXT("[%s] Requested auto generated world map scene capture"), *GetNameSafe(this));

      const uint64 sceneCaptureStartTime = FPlatformTime::Cycles64();

      FVector sceneCaptureOrigin = mapAreaBounds.GetCenter();
      sceneCaptureOrigin.Z = mapAreaBounds.Max.Z + MapRenderTextureHeightOffset;
      _mapSceneCaptureComponent->SetWorldLocation(sceneCaptureOrigin);
      _mapSceneCaptureComponent->SetWorldRotation(FRotationMatrix::MakeFromXZ(FVector(0, 0, -1), FVector(0, -1, 0)).Rotator());
      _mapSceneCaptureComponent->CaptureSource = MapRenderTextureCaptureSource;

      auto calcProjMatrix = [](FMatrix& outProjectionMatrix, const FBox& mapBoundingBox, float mapAreaHeight)
      {
         const FVector size = mapBoundingBox.GetSize();
         const FVector extent = mapBoundingBox.GetExtent();

         FVector mapSizeMin(mapBoundingBox.Min.X, mapBoundingBox.Min.Y, mapBoundingBox.Min.Z);
         FVector mapSizeMax(mapBoundingBox.Max.X, mapBoundingBox.Max.Y, mapBoundingBox.Max.Z);

         FVector mapSize = (mapSizeMax - mapSizeMin);
         mapSize.X = FMath::Abs(mapSize.X);
         mapSize.Y = FMath::Abs(mapSize.Y);
         mapSize.Z = mapAreaHeight;

         const float viewportWidth = extent.X * 2;
         const float viewportHeight = extent.Y * 2;
         const bool useYAxis = (mapSize.X / mapSize.Y) <= 1;
         const float mapAxisSize = useYAxis ? mapSize.X : mapSize.Y;
         const uint32 viewportAxisSize = useYAxis ? viewportWidth : viewportHeight;
         const float orthoZoom = mapAxisSize / viewportAxisSize / 2.f;
         const float orthoWidth = FMath::Max(1.f, viewportWidth * orthoZoom);
         const float orthoHeight = FMath::Max(1.f, viewportHeight * orthoZoom);

         const double zOffset = (mapSize.Z > 0) ? mapSize.Z : (UE_FLOAT_HUGE_DISTANCE / 2.0);
         outProjectionMatrix = FReversedZOrthoMatrix(orthoWidth, orthoHeight, 0.5f / zOffset, 0.0);

         ensureMsgf(!outProjectionMatrix.ContainsNaN(), TEXT("Nans found on ProjectionMatrix"));
         if (outProjectionMatrix.ContainsNaN())
         {
            outProjectionMatrix.SetIdentity();
         }
      };

      float areaHeight = FMath::Max(0.0f, mapAreaBounds.GetSize().Z + MapRenderTextureHeightOffset);
      if (areaHeight <= 0)
      {
         areaHeight = -1.0f;
      }

      _mapSceneCaptureComponent->OrthoWidth = mapAreaBounds.GetExtent().X * 2.0f;
      _mapSceneCaptureComponent->bUseCustomProjectionMatrix = true;
      calcProjMatrix(_mapSceneCaptureComponent->CustomProjectionMatrix, mapAreaBounds, areaHeight);

      _mapSceneCaptureComponent->MaxViewDistanceOverride = (MapType != ETATWorldMapBoundaryType::Primary) ? areaHeight : -1.0f;

      _mapSceneCaptureComponent->TextureTarget = _mapRenderTarget;
      _mapSceneCaptureComponent->CaptureScene();

      _mapRenderTargetCaptured = true;
      updatedRenderTarget = true;

      const uint64 sceneCaptureDuration = FPlatformTime::Cycles64() - sceneCaptureStartTime;
      const double sceneCaptureDurationSeconds = FPlatformTime::ToSeconds64(sceneCaptureDuration);
      UE_LOG(LogTATWorldMapBoundary, Log, TEXT("UpdateMapTextureUsingRenderTarget took %llu cycles (%.4f seconds)"), sceneCaptureDuration, sceneCaptureDurationSeconds);
   }

   OnUpdateMapTexture.Broadcast(_mapRenderTarget, mapAreaBounds, updatedRenderTarget);
}

void ATATWorldMapBoundary::_OnBoxBeginOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex, bool fromSweep, const FHitResult& sweepResult)
{
   if (!AutoSwitchToMap)
   {
      return;
   }

   if (ACharacter* otherChar = Cast<ACharacter>(otherActor))
   {
      ATATPlayerController* pc = otherChar->GetController<ATATPlayerController>();
      if (pc != nullptr && pc->IsLocalPlayerController())
      {
         if (UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
         {
            worldMapSubsystem->NotifyLocalPlayerInteriorMapBeginOverlap(this, pc);
         }
      }
   }
}

void ATATWorldMapBoundary::_OnBoxEndOverlap(UPrimitiveComponent* overlappedComponent, AActor* otherActor, UPrimitiveComponent* otherComp, int32 otherBodyIndex)
{
   if (!AutoSwitchToMap)
   {
      return;
   }

   if (ACharacter* otherChar = Cast<ACharacter>(otherActor))
   {
      ATATPlayerController* pc = otherChar->GetController<ATATPlayerController>();
      if (pc != nullptr && pc->IsLocalPlayerController())
      {
         if (UTATWorldMapSubsystem* worldMapSubsystem = GetWorld()->GetSubsystem<UTATWorldMapSubsystem>())
         {
            worldMapSubsystem->NotifyLocalPlayerInteriorMapEndOverlap(this, pc);
         }
      }
   }
}
