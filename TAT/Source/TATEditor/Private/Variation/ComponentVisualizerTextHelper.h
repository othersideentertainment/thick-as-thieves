// (c) 2021-2024 OtherSide Entertainment, Inc
// SPDX-License-Identifier: MIT

#pragma once

// ue4
#include "CanvasItem.h"
#include "SceneManagement.h"
#include "Engine/Canvas.h"
#include "Math/Box.h"

namespace VisualizerHelper
{
   struct FTextDrawer
   {
      FTextDrawer(const FVector& worldPosition, const FSceneView* view, FCanvas* canvas)
         : _canvas(canvas)
      {
         check(view != nullptr && canvas != nullptr);
         _isValid = view->WorldToPixel(worldPosition, _pixelLoc)
            && FVector::DotProduct((worldPosition - view->ViewLocation).GetSafeNormal(), view->GetViewDirection()) > 0;
         if (_isValid)
         {
            _pixelLoc /= canvas->GetDPIScale();
         }
      }

      bool IsValid() const { return _isValid; }
      explicit operator bool() const { return _isValid; }

      void AddTextItem(FStringView string, FColor color)
      {
         const FVector2D kPositionOffset = FVector2D(0.0f, 15.0f);
         FCanvasTextStringViewItem textItem(_pixelLoc, string, GEngine->GetMediumFont(), color);
         textItem.Position += (_positionOffsetIndex * kPositionOffset);
         textItem.EnableShadow(FLinearColor::Black);
         textItem.bCentreX = true;
         textItem.Draw(_canvas);
         ++_positionOffsetIndex;
      };
   private:
      FVector2D _pixelLoc;
      FCanvas* _canvas;
      int _positionOffsetIndex = 1;
      bool _isValid;
   };
}

